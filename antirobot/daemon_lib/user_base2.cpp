#include "user_base2.h"

#include "config_global.h"
#include "convert_factors.h"
#include "eventlog_err.h"
#include "uid.h"

#include <contrib/libs/kyotocabinet/kcpolydb.h>

#include <util/datetime/base.h>
#include <util/folder/dirut.h>
#include <util/generic/hash.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/system/event.h>
#include <util/system/fs.h>
#include <util/system/mutex.h>
#include <util/system/rwlock.h>
#include <util/system/thread.h>
#include <util/system/tls.h>

namespace NAntiRobot {

    namespace {
        const char* DATABASE_DIR = "user_base";
        const char* DATABASE_NAME = "userbase";

        const TStringBuf SPECIAL_KEYS_PREFIX = "SPECIAL_";
        const TStringBuf DB_VERSION_KEY = "SPECIAL_DbVersion";
        const TStringBuf CUR_DB_VERSION = "8";

        const TDuration FLUSH_DIRTY_INTERVAL = TDuration::Seconds(30);
        const TDuration DIRTY_PERIOD = TDuration::Seconds(30);

        typedef TThreatLevel TCacheItem;

        typedef TSimpleSharedPtr<TCacheItem> TUidDataPtr;
        typedef THashMap<TUid, TUidDataPtr, TUidHash> TUidDataList;
        typedef TVector<TUid> TUidVector;

        struct TTlTimeInfo {
            ui64 TimeStamp;
            int AggregationLevel;
            TUidDataList::iterator ToTl;

            bool operator<(const TTlTimeInfo& rhs) const {
                if (AggregationLevel < 1 && rhs.AggregationLevel >= 1)
                    return true;

                if (AggregationLevel >= 1 && rhs.AggregationLevel < 1)
                    return false;

                return TimeStamp < rhs.TimeStamp;
            }
        };

        typedef TVector<TTlTimeInfo> TTlTimeInfoVector;


        struct TUserTimeInfo {
            enum {MAX_KEY_SIZE = 25};
            TInstant TimeStamp;
            char Key[MAX_KEY_SIZE];
            size_t KeySize;
            bool operator<(const TUserTimeInfo& rhs) const {
                return TimeStamp < rhs.TimeStamp;
            }
        };

        class TKyotoLogger : public kyotocabinet::BasicDB::Logger {
            virtual void log(const char* file, int32_t line, const char* func, Kind kind, const char* message) {
                EVLOG_MSG << (kind == ERROR ? EVLOG_ERROR : EVLOG_WARNING) << "kyotocabinet: "sv << file << ':' << line << ' ' << message;
            }
        };
    }

    struct TDbDestroyer {
        static inline void Destroy(kyotocabinet::HashDB* db) noexcept {
            db->close();
            delete db;
        }
    };

    typedef THolder<kyotocabinet::HashDB, TDbDestroyer> TDbHolder;

    class TDirtyItems {
        TMutex Lock;
        typedef THashMap<TUid, TInstant, TUidHash> TUidSet;

        TUidSet UidSet;
    public:
        TDirtyItems()
            : UidSet(ANTIROBOT_DAEMON_CONFIG.MaxSafeUsers)
        {
        }

        inline void Add(const TUid& uid) {
            TGuard<TMutex> guard(Lock);
            UidSet[uid] = TInstant::Now();
        }

        void MoveAll(TUidVector& list) {
            TGuard<TMutex> guard(Lock);

            list.reserve(UidSet.size());
            for (TUidSet::iterator it = UidSet.begin(); it != UidSet.end(); it++)
                list.push_back(it->first);

            UidSet.clear();
        }

        bool Has(const TUid& uid) {
            TGuard<TMutex> guard(Lock);
            return UidSet.find(uid) != UidSet.end();
        }

        size_t Size() const {
            return UidSet.size();
        }

        TInstant GetTime(const TUid& uid) {
            TGuard<TMutex> guard(Lock);
            TUidSet::const_iterator toUid =  UidSet.find(uid);
            if (toUid != UidSet.end())
                return toUid->second;
            return TInstant();
        }
    };


    class TUidsInUse {
    public:
        void SetInUse(const TUid& uid) {
            TGuard<TMutex> lock(Mutex);
            Uids.insert(uid);
        }

        void SetNotInUse(const TUid& uid) {
            TGuard<TMutex> lock(Mutex);
            Uids.erase(uid);
        }

        bool IsInUse(const TUid& uid) {
            TGuard<TMutex> lock(Mutex);
            return Uids.find(uid) != Uids.end();
        }

        size_t Size() const {
            return Uids.size();
        }

    private:
        THashSet<TUid, TUidHash> Uids;
        TMutex Mutex;
    };

    class TUserBase2::TImpl {
        friend class TUserBase2::TValue;

        class TSyncThread : public TThread {
            TUserBase2::TImpl& UserBase;
            TSystemEvent Quit;
            TDuration SyncInterval;
        public:
            TSyncThread(TUserBase2::TImpl& userBase)
                : TThread(&ThreadProc, this)
                , UserBase(userBase)
                , Quit(TSystemEvent::rAuto)
                , SyncInterval(ANTIROBOT_DAEMON_CONFIG.DbSyncInterval)
            {
            }

            static void* ThreadProc(void* _this) {
                static_cast<TSyncThread*>(_this)->Exec();
                return 0;
            }

            void Exec() {
                while(!Quit.WaitT(TDuration::Seconds(1))) {
                    try {
                        UserBase.ClearCache();
                    } catch (...) {
                        EVLOG_MSG << EVLOG_ERROR << "Sync thread: " << CurrentExceptionMessage();
                    }
                }
            }

            inline void Stop() {
                Quit.Signal();
            }

        };

        struct TPerUidHashData {
            TMutex Mutex;
            TCacheItem* ThreatLevelPtr;
            TCacheItem ThreatLevelDummy;

            TPerUidHashData() {
                Reset();
            }

            void Reset() {
                ThreatLevelPtr = NULL;
                ThreatLevelDummy.Clear();
            }
        };

        TUidDataList UidDataList;
        TUidsInUse UidsInUse;
        bool UseBdb;
        TString DbDir;
        TKyotoLogger KyotoLogger;
        TDbHolder Db_;
        size_t DbSize;
        //TMutex Mutex;
        TRWMutex DbMutex;
        TRWMutex CacheMutex;
        static const size_t NUM_UID_DATA = 1000;
        TPerUidHashData PerUidHashData[NUM_UID_DATA];
        ui64 NumWriteErrors;
        ui64 NumSyncErrors;
        TAtomic NumCacheHits;
        TAtomic NumCacheMiss;
        TSyncThread SyncThread;
        TTlTimeInfoVector TlTimes;

    public:
        TImpl(const TString& baseDir)
            : UidDataList(ANTIROBOT_DAEMON_CONFIG.MaxSafeUsers + ANTIROBOT_DAEMON_CONFIG.UsersToDelete)
            , UseBdb(ANTIROBOT_DAEMON_CONFIG.UseBdb)
            , DbSize(0)
            , NumWriteErrors(0)
            , NumSyncErrors(0)
            , NumCacheHits(0)
            , NumCacheMiss(0)
            , SyncThread(*this)
        {
            if (UseBdb) {
                DbDir = baseDir + '/' + DATABASE_DIR;
                MakeDirIfNotExist(DbDir.c_str());
                OpenBase();
                InitFactorsConvert();
                LoadCacheFromDb();
                SyncThread.Start();
            }
        }

        ~TImpl() {
            if (SyncThread.Running()) {
                SyncThread.Stop();
                SyncThread.Join();
                SaveCacheToDb();
            }
        }

        void DeleteOldUsers(size_t& compressedSize, size_t robotsSize) {
            if (UseBdb)
                return;

            if (UidDataList.size() < ANTIROBOT_DAEMON_CONFIG.MaxSafeUsers)
                return;

            TWriteGuard guard(CacheMutex);

            size_t totalMem = compressedSize
                + UidDataList.size() * (sizeof(TUidDataList::value_type) + 8)
                + robotsSize;

            EVLOG_MSG << "Deleting old users, num users: " << UidDataList.size() << ", mem: " << totalMem;

            TVector<TTlTimeInfo> tlTimes;
            tlTimes.reserve(UidDataList.size());
            for (TUidDataList::iterator toTl = UidDataList.begin(); toTl != UidDataList.end(); ++toTl) {
                if (UidsInUse.IsInUse(toTl->first))
                    continue;

                TTlTimeInfo tlTimeInfo;
                tlTimeInfo.TimeStamp = toTl->second->LastTimeStamp().GetValue();
                tlTimeInfo.AggregationLevel = toTl->first.GetAggregationLevel();
                tlTimeInfo.ToTl = toTl;
                tlTimes.push_back(tlTimeInfo);
            }

            std::sort(tlTimes.begin(), tlTimes.end());

            size_t usersToDelete = ANTIROBOT_DAEMON_CONFIG.UsersToDelete;
            for (size_t i = 0; i < usersToDelete; ++i) {
                compressedSize -= tlTimes[i].ToTl->second->GetCompressedDataSize();
                UidDataList.erase(tlTimes[i].ToTl);
            }
        }

        TValue Get(const TUid& uid) {
            TPerUidHashData& uidData = GetPerUidHashData(uid);
            TCacheItem*& threatLevelPtr = uidData.ThreatLevelPtr;
            TValue result(*this, uid); // locks mutex here
            try {
                bool inCache = false;
                {
                    TReadGuard guard (CacheMutex);
                    TUidDataList::iterator toTl = UidDataList.find(uid);
                    if (toTl != UidDataList.end()) {
                        threatLevelPtr = toTl->second.Get();
                        AtomicIncrement(NumCacheHits);
                        inCache = true;
                    }
                }

                if (!inCache) {
                    AtomicIncrement(NumCacheMiss);
                    threatLevelPtr = new TCacheItem();

                    {
                        TWriteGuard guard (CacheMutex);
                        UidDataList[uid].Reset(threatLevelPtr);
                    }
                }

                return result;
            } catch(...) {
                EVLOG_MSG << CurrentExceptionMessage();
                throw;
            }
        }

        void PrintStats(IOutputStream& out) const {
            out << "<userbase_write_errors>" << NumWriteErrors<< "</userbase_write_errors>";
            out << "<cache_hits>" << AtomicGet(NumCacheHits) << "</cache_hits>";
            out << "<cache_miss>" << AtomicGet(NumCacheMiss) << "</cache_miss>";
            if (UseBdb) {
                out << "<userbase_count>" << DbSize << "</userbase_count>";
                out << "<cache_size>" << UidDataList.size() << "</cache_size>";
                out << "<sync_errors>" << NumSyncErrors << "</sync_errors>";
                out << "<in_use>" << UidsInUse.Size() << "</in_use>";
            } else {
                out << "<userbase_count>" << UidDataList.size() << "</userbase_count>";
            }
        }

        void PrintMemStats(IOutputStream& out) const {
            if (UseBdb)
                return;

            TReadGuard guard(CacheMutex);

            size_t uncompressedSize = UidDataList.size() * TThreatLevel::GetUncompressedDataSize();

            out << "num tls: " << UidDataList.size() << ", uncompressed size: " << uncompressedSize << Endl
                << "rest tls: " << UidDataList.size() * sizeof(TThreatLevel) << ", storage: " << UidDataList.size() * (sizeof(TUidDataList::value_type) + 8) << Endl;
        }

        void DumpCache(IOutputStream& out) {
            TReadGuard guard(CacheMutex);

            for (TUidDataList::const_iterator toTl = UidDataList.begin(); toTl != UidDataList.end(); ++toTl) {
                Y_ASSERT(!!toTl->second);

                out << toTl->first << '\t' << toTl->second->LastTimeStamp().GetValue() << '\n';
            }
        }

        void SaveCacheToDb() {
            if (!UseBdb)
                return;

            size_t oldDbSize = DbSize;
            ui64 oldNumWriteErrors = NumWriteErrors;

            TWriteGuard dbGuard(DbMutex);
            TReadGuard cacheGuard(CacheMutex);

            TInstant startTime = TInstant::Now();

            OpenFreshDb();
            for (TUidDataList::iterator toTl = UidDataList.begin(); toTl != UidDataList.end(); ++toTl) {
                Y_ASSERT(!!toTl->second);
                toTl->second->SetFlushed(startTime);
                WriteData(toTl->first, *toTl->second);
            }

            Sync();
            DbSize = Db_->count();
            TInstant endTime = TInstant::Now();

            Cerr << "Save cache, size = " << UidDataList.size() << ", old db size: " << oldDbSize << ", new db size: " << DbSize
                << ", write errors: " << (NumWriteErrors - oldNumWriteErrors)
                << ", total time: " << (endTime - startTime)
                << Endl;
            EVLOG_MSG << EVLOG_ERROR << "Save cache, size = " << UidDataList.size() << ", old db size: " << oldDbSize << ", new db size: " << DbSize
                << ", write errors: " << (NumWriteErrors - oldNumWriteErrors)
                << ", total time: " << (endTime - startTime)
                << Endl;
        }


        void LoadCacheFromDb() {
            class TLoadCacheVisitor : public kyotocabinet::DB::Visitor {
            public:
                TLoadCacheVisitor(TUidDataList& cache)
                    : Cache(cache)
                {
                }

            private:
                const char* visit_full(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz, size_t* sp) {
                    if (TStringBuf(kbuf, ksiz).StartsWith(SPECIAL_KEYS_PREFIX))
                        return NOP;

                    if (ksiz != sizeof(TUid)) {
                        return NOP;
                    }

                    TMemoryInput mi(vbuf, vsiz);

                    TSimpleSharedPtr<TCacheItem> tlPtr = new TCacheItem();

                    ::Load(&mi, *tlPtr);

                    const TUid* uid = reinterpret_cast<const TUid*>(kbuf);
                    Cache[*uid] = tlPtr;

                    return NOP;
                }

                TUidDataList& Cache;
                TThreatLevel TlDummy;
            };

            if (!UseBdb)
                return;

            TReadGuard dbGuard(DbMutex);
            TWriteGuard cacheGuard(CacheMutex);

            size_t oldDbSize = DbSize;
            size_t oldCacheSize = UidDataList.size();

            TInstant startTime = TInstant::Now();

            for (size_t i = 0; i < NUM_UID_DATA; ++i)
                PerUidHashData[i].Reset();

            UidDataList.clear();
            TLoadCacheVisitor loadCacheVisitor(UidDataList);
            Db_->iterate(&loadCacheVisitor, false);
            Sync();

            TInstant endTime = TInstant::Now();

            DbSize = Db_->count();

            Cerr << "Load cache, old size: " << oldCacheSize << ", new size: " << UidDataList.size()
                << ", old db size: " << oldDbSize << ", new db size: " << DbSize
                << ", total time: " << (endTime - startTime)
                << Endl;
            EVLOG_MSG << EVLOG_ERROR << "Load cache, old size: " << oldCacheSize << ", new size: " << UidDataList.size()
                << ", old db size: " << oldDbSize << ", new db size: " << DbSize
                << ", total time: " << (endTime - startTime)
                << Endl;
        }

    private:
        bool TryOpenBase(const TString& baseFile) {
            using namespace kyotocabinet;

            const int DB_FLAGS = HashDB::OWRITER | HashDB::OCREATE | HashDB::ONOREPAIR;

            Db_.Reset(new kyotocabinet::HashDB());
            Db_->tune_alignment(8);
            Db_->tune_buckets(5LL * 1000 * 1000);
            Db_->tune_defrag(4);
            Db_->tune_logger(&KyotoLogger, TKyotoLogger::WARN | TKyotoLogger::ERROR);

            if (!Db_->open(baseFile.c_str(), DB_FLAGS)) {
                EVLOG_MSG << EVLOG_ERROR << "Error on openning database: " << Db_->error().message();
                return false;
            }

            TTempBuf valueBuf;
            if (Db_->count() > 0) {
                int32_t valueSize = Db_->get(~DB_VERSION_KEY, +DB_VERSION_KEY, valueBuf.Data(), valueBuf.Size());
                if (valueSize < 0 || TStringBuf(valueBuf.Data(), valueSize) != CUR_DB_VERSION) {
                    EVLOG_MSG << EVLOG_ERROR << "Incorrect db version, creating new db";
                    return false;
                }
            } else {
                if (!Db_->set(~DB_VERSION_KEY, +DB_VERSION_KEY, ~CUR_DB_VERSION, +CUR_DB_VERSION))
                    EVLOG_MSG << EVLOG_ERROR << "Couldn't write database version to db:" << Db_->error().message();
            }

            Sync();

            DbSize = Db_->count();
            return true;
        }

        void OpenBase() {
            TString baseFile = DbDir + "/" + DATABASE_NAME;
            if (!TryOpenBase(baseFile)) {
                EVLOG_MSG << EVLOG_ERROR << "Trying to recreate the database";
                NFs::Remove(baseFile.c_str());
                if (!TryOpenBase(baseFile))
                    throw yexception() << "Cannot open user database";
            }
            EVLOG_MSG << "Userbase is opened";

        }

        bool OpenFreshDb() {
            TString baseFile = DbDir + "/" + DATABASE_NAME;
            TString bakFile = baseFile + ".bak";

            if (!Db_->close()) {
                EVLOG_MSG << "Closing database failed (ignored)";
                Db_.Release();
            }

            NFs::Rename(baseFile.c_str(), bakFile.c_str());
            EVLOG_MSG << EVLOG_ERROR << "Switch to fresh database";
            return TryOpenBase(baseFile);
        }

        void WriteData(const TUid& uid, const TThreatLevel& threatLevel) {
            TTempBufOutput out;
            try {
                ::Save(&out, threatLevel);
            } catch(...) {
                ythrow yexception() << "buf error in WriteData: " << CurrentExceptionMessage();
            }

            if (!Db_->set(reinterpret_cast<const char*>(&uid), sizeof(TUid), out.Data(), out.Filled())) {
                ++NumWriteErrors;
                EVLOG_MSG << "Db write error: " << Db_->error().message();
            }
        }

        void AcquireValue(const TUid& uid) {
            GetPerUidHashData(uid).Mutex.Acquire();
            UidsInUse.SetInUse(uid);
        }

        void LeaveValue(const TUid& uid) {
            UidsInUse.SetNotInUse(uid);
            GetPerUidHashData(uid).Mutex.Release();
        }

        void Sync() {
            using namespace kyotocabinet;

            if (!Db_->synchronize(false)) {
                BasicDB::Error err = Db_->error();
                if (err.code() == BasicDB::Error::BROKEN) {
                    EVLOG_MSG << EVLOG_ERROR << "Needs to recovery db, will fresh it";
                    OpenFreshDb(); //TODO: check result
                }
                ++NumSyncErrors;
                EVLOG_MSG << EVLOG_ERROR << "DB synchronize error: " << err.message();
            }
        }

        void ClearCache() {
            if (UidDataList.size() < ANTIROBOT_DAEMON_CONFIG.MaxSafeUsers)
                return;

            if (UidDataList.size() < ANTIROBOT_DAEMON_CONFIG.UsersToDelete)
                return;

            TWriteGuard guard(CacheMutex);
            TInstant s = TInstant::Now();

            TlTimes.clear();
            TlTimes.reserve(UidDataList.size());

            for (TUidDataList::iterator toTl = UidDataList.begin(); toTl != UidDataList.end(); ++toTl) {
                Y_ASSERT(!!toTl->second);

                if (!UidsInUse.IsInUse(toTl->first)) {
                    TTlTimeInfo tlTimeInfo;
                    tlTimeInfo.TimeStamp = toTl->second->LastTimeStamp().GetValue();
                    tlTimeInfo.AggregationLevel = toTl->first.GetAggregationLevel();
                    tlTimeInfo.ToTl = toTl;
                    TlTimes.push_back(tlTimeInfo);
                }
            }

            std::sort(TlTimes.begin(), TlTimes.end());
            size_t deleteCount = TlTimes.size() > ANTIROBOT_DAEMON_CONFIG.MaxSafeUsers ?
                  ANTIROBOT_DAEMON_CONFIG.UsersToDelete + (TlTimes.size() - ANTIROBOT_DAEMON_CONFIG.MaxSafeUsers)
                : Min(ANTIROBOT_DAEMON_CONFIG.UsersToDelete, TlTimes.size());

            for (size_t i = 0; i < deleteCount; ++i) {
                UidDataList.erase(TlTimes[i].ToTl);
            }

            if (ANTIROBOT_DAEMON_CONFIG.DebugOutput) {
                Cerr << "ClearCache() time: " << TDuration::MicroSeconds(TInstant::Now() - s) << "\tdeleted: " << deleteCount << " of " << TlTimes.size() << Endl;
            }

        }

        TPerUidHashData& GetPerUidHashData(const TUid& uid) {
            return PerUidHashData[uid.Id % NUM_UID_DATA];
        }

        friend class TSyncThread;
    };

    TUserBase2::TUserBase2(const TString& baseDir)
        : Impl(new TImpl(baseDir))
    {
    }

    TUserBase2::~TUserBase2() {
    }

    TUserBase2::TValue TUserBase2::Get(const TUid& uid) {
        return Impl->Get(uid);
    }

    void TUserBase2::DeleteOldUsers(size_t& compressedSize, size_t robotsSize) {
        Impl->DeleteOldUsers(compressedSize, robotsSize);
    }

    void TUserBase2::PrintStats(IOutputStream& out) const {
        Impl->PrintStats(out);
    }

    void TUserBase2::PrintMemStats(IOutputStream& out) const {
        Impl->PrintMemStats(out);
    }

    void TUserBase2::DumpCache(IOutputStream& out) const {
        Impl->DumpCache(out);
    }

    TUserBase2::TValue::TValue(TImpl& userBase, const TUid& uid)
        : UserBase(userBase)
        , Uid(uid)
        , Ownership(true)
    {
        UserBase.AcquireValue(Uid);
    }

    TUserBase2::TValue::TValue(const TValue& copy)
        : UserBase(copy.UserBase)
        , Uid(copy.Uid)
        , Ownership(true)
    {
        copy.Ownership = false;
    }

    TUserBase2::TValue::~TValue() {
        if (Ownership) {
            UserBase.LeaveValue(Uid);
        }
    }

    TThreatLevel* TUserBase2::TValue:: operator->() const {
        return UserBase.GetPerUidHashData(Uid).ThreatLevelPtr;
    }
}

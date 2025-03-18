#include "user_base.h"

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
#include <util/system/sanitizers.h>
#include <util/system/thread.h>
#include <util/system/tls.h>

namespace NAntiRobot {

    namespace {
        const char* DATABASE_DIR = "user_base";
        const char* DATABASE_NAME = "userbase";

        const TStringBuf SPECIAL_KEYS_PREFIX = "SPECIAL_";
        const TStringBuf DB_VERSION_KEY = "SPECIAL_DbVersion";
        const TStringBuf CUR_DB_VERSION = "9";

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

            static inline bool LessForFlushDirty(const TTlTimeInfo& lhs, const TTlTimeInfo& rhs) {
                if (lhs.AggregationLevel >= 1 && rhs.AggregationLevel < 1)
                    return true;

                if (lhs.AggregationLevel < 1 && rhs.AggregationLevel >= 1)
                    return false;


                return lhs.TimeStamp < rhs.TimeStamp;
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
        public:
            TKyotoLogger(TAtomic& numWarningLogs, TAtomic& numErrorLogs)
                : kyotocabinet::BasicDB::Logger()
                , NumWarningLogs(numWarningLogs)
                , NumErrorLogs(numErrorLogs)
            {
            }

        private:
            void log(const char* file, int32_t line, const char*, Kind kind, const char* message) override {
                EVLOG_MSG << (kind == ERROR ? EVLOG_ERROR : EVLOG_WARNING) << "kyotocabinet: "sv << file << ':' << line << ' ' << message;

                if (kind == WARN) {
                    AtomicIncrement(NumWarningLogs);
                } else if (kind == ERROR) {
                    AtomicIncrement(NumErrorLogs);
                }
            }

        private:
            TAtomic& NumWarningLogs;
            TAtomic& NumErrorLogs;
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

    class TUserBase::TImpl {
        friend class TUserBase::TValue;

        class TSyncThread : public TThread {
            TUserBase::TImpl& UserBase;
            TSystemEvent Quit;
            TDuration SyncInterval;
        public:
            TSyncThread(TUserBase::TImpl& userBase)
                : TThread(&ThreadProc, this)
                , UserBase(userBase)
                , Quit(TSystemEvent::rAuto)
                , SyncInterval(ANTIROBOT_DAEMON_CONFIG.DbSyncInterval)
            {
            }

            static void* ThreadProc(void* _this) {
                TThread::SetCurrentThreadName("UserBaseSync");
                static_cast<TSyncThread*>(_this)->Exec();
                return nullptr;
            }

            void Exec() {
                TInstant nextFlush = TInstant::Now() + SyncInterval;
                while(!Quit.WaitT(TDuration::Seconds(1))) {
                    try {
                        TInstant now = TInstant::Now();

                        UserBase.ClearCache();

                        if (now >= nextFlush)  {
                            UserBase.FlushDirtyItems(ANTIROBOT_DAEMON_CONFIG.MaxItemsToSync);
                            nextFlush = now + SyncInterval;
                        }

                        UserBase.CleanupDb();
                    } catch (...) {
                        EVLOG_MSG << EVLOG_ERROR << "Sync thread: " << CurrentExceptionMessage();
                    }
                    AtomicSet(UserBase.IsCleaning, 0);
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
        };

        TUidDataList UidDataList;
        TDirtyItems DirtyItems;
        TUidsInUse UidsInUse;
        bool UseBdb;
        TString DbDir;
        TKyotoLogger KyotoLogger{NumWarningLogs, NumErrorLogs};
        TDbHolder Db_;
        size_t DbSize;
        TRWMutex DbCleanupMutex;
        TRWMutex CacheMutex;
        static const size_t NUM_UID_DATA = 1000;
        TPerUidHashData PerUidHashData[NUM_UID_DATA];
        TAtomic NumUidInBase;
        TAtomic NumUidNotInBase;
        ui64 NumWriteErrors;
        ui64 NumWriteErrorsSinceLastDbReset;
        ui64 NumSyncErrors;
        TAtomic NumCacheHits;
        TAtomic NumCacheMiss;
        TInstant LastFlushTime;
        TDuration LastFlushDuration;
        ui64 LastFlushNumItems;
        ui64 LastFlushDirtySetSize;
        TSyncThread SyncThread;
        TTlTimeInfoVector TlTimes;
        TVector<TUserTimeInfo> UserTimes;
        TAtomic NumWarningLogs;
        TAtomic NumErrorLogs;
        TAtomic IsCleaning;
        TAtomic DbGetExceptions;
        TAtomic SkippedReads;

    public:
        TImpl(const TString& baseDir)
            : UidDataList(ANTIROBOT_DAEMON_CONFIG.MaxSafeUsers + ANTIROBOT_DAEMON_CONFIG.UsersToDelete)
            , UseBdb(ANTIROBOT_DAEMON_CONFIG.UseBdb)
            , DbSize(0)
            , NumUidInBase(0)
            , NumUidNotInBase(0)
            , NumWriteErrors(0)
            , NumWriteErrorsSinceLastDbReset(0)
            , NumSyncErrors(0)
            , NumCacheHits(0)
            , NumCacheMiss(0)
            , LastFlushNumItems(0)
            , LastFlushDirtySetSize(0)
            , SyncThread(*this)
            , NumWarningLogs(0)
            , NumErrorLogs(0)
            , IsCleaning(0)
            , DbGetExceptions(0)
            , SkippedReads(0)
        {
            if (UseBdb) {
                DbDir = baseDir + '/' + DATABASE_DIR;
                MakeDirIfNotExist(DbDir.c_str());
                OpenBase();
                InitFactorsConvert();
                SyncThread.Start();
            }
        }

        ~TImpl() {
            if (SyncThread.Running()) {
                SyncThread.Stop();
                SyncThread.Join();
                FlushDirtyItems(DirtyItems.Size());
            }
        }

        void DeleteOldUsers() {
            if (UseBdb) {
                return;
            }

            if (UidDataList.size() < ANTIROBOT_DAEMON_CONFIG.MaxSafeUsers) {
                return;
            }

            TWriteGuard guard(CacheMutex);

            EVLOG_MSG << "Deleting old users, num users: " << UidDataList.size();

            TVector<TTlTimeInfo> tlTimes;
            tlTimes.reserve(UidDataList.size());
            for (TUidDataList::iterator toTl = UidDataList.begin(); toTl != UidDataList.end(); ++toTl) {
                if (UidsInUse.IsInUse(toTl->first)) {
                    continue;
                }

                TTlTimeInfo tlTimeInfo;
                tlTimeInfo.TimeStamp = toTl->second->LastTimeStamp().GetValue();
                tlTimeInfo.AggregationLevel = toTl->first.GetAggregationLevel();
                tlTimeInfo.ToTl = toTl;
                tlTimes.push_back(tlTimeInfo);
            }

            std::sort(tlTimes.begin(), tlTimes.end());

            size_t usersToDelete = Min(ANTIROBOT_DAEMON_CONFIG.UsersToDelete, tlTimes.size());
            for (size_t i = 0; i < usersToDelete; ++i) {
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

                    if (UseBdb) {
                        TryReadFromDb(uid);
                        threatLevelPtr = new TCacheItem(uidData.ThreatLevelDummy); //TODO: fix trash
                    } else {
                        threatLevelPtr = new TCacheItem;
                    }

                    {
                        TWriteGuard guard (CacheMutex);
                        UidDataList[uid].Reset(threatLevelPtr);
                    }
                }

                threatLevelPtr->SetFlushed(TInstant::Now());
                return result;
            } catch(...) {
                EVLOG_MSG << CurrentExceptionMessage();
                throw;
            }
        }

        void PrintStats(TStatsWriter& out) const {
            out.WriteScalar("userbase_hits", AtomicGet(NumUidInBase))
               .WriteScalar("userbase_miss", AtomicGet(NumUidNotInBase))
               .WriteScalar("userbase_write_errors", NumWriteErrors)
               .WriteScalar("userbase_write_errors_last", NumWriteErrorsSinceLastDbReset)
               .WriteScalar("userbase_cache_hits", AtomicGet(NumCacheHits))
               .WriteScalar("userbase_cache_miss", AtomicGet(NumCacheMiss))
               .WriteScalar("userbase_warning_logs", AtomicGet(NumWarningLogs))
               .WriteScalar("userbase_error_logs", AtomicGet(NumErrorLogs))
               .WriteHistogram("userbase_is_cleaning", AtomicGet(IsCleaning))
               .WriteScalar("userbase_db_get_exceptions", AtomicGet(DbGetExceptions))
               .WriteScalar("userbase_skipped_reads", AtomicGet(SkippedReads));

            if (UseBdb) {
                out.WriteHistogram("userbase_count", DbSize)
                   .WriteHistogram("userbase_cache_size", UidDataList.size())
                   .WriteHistogram("userbase_flush_time", LastFlushTime.MicroSeconds())
                   .WriteHistogram("userbase_flush_dur", LastFlushDuration.MicroSeconds())
                   .WriteHistogram("userbase_flush_items", LastFlushNumItems)
                   .WriteHistogram("userbase_flush_dirty_size", LastFlushDirtySetSize)
                   .WriteScalar("userbase_sync_errors", NumSyncErrors)
                   .WriteHistogram("userbase_in_use", UidsInUse.Size());
            } else {
                out.WriteHistogram("userbase_count", UidDataList.size());
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

                out << toTl->first << '\t' << toTl->second->LastTimeStamp().GetValue() << '\t'
                    << DirtyItems.GetTime(toTl->first).GetValue() << '\n';
            }
        }

        void CleanupDb() {
            class TGetTimeVisitor : public kyotocabinet::DB::Visitor {
            public:
                TGetTimeVisitor(TVector<TUserTimeInfo>& userTimes)
                    : UserTimes(userTimes)
                {
                }

            private:
                TVector<TUserTimeInfo>& UserTimes;

                const char* visit_full(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz, size_t*) override {
                    if (TStringBuf(kbuf, ksiz).StartsWith(SPECIAL_KEYS_PREFIX))
                        return NOP;

                    if (ksiz > TUserTimeInfo::MAX_KEY_SIZE) {
                        EVLOG_MSG << "key size exceeded: " << TStringBuf(kbuf, ksiz);
                        return NOP;
                    }

                    TMemoryInput mi(vbuf, vsiz);
                    ui32 FactorsVersion;
                    TInstant TimeStamp;
                    ::Load(&mi, FactorsVersion);
                    ::Load(&mi, TimeStamp);

                    TUserTimeInfo userTimeInfo;
                    userTimeInfo.TimeStamp = TimeStamp;
                    memcpy(userTimeInfo.Key, kbuf, ksiz);
                    userTimeInfo.KeySize = ksiz;
                    UserTimes.push_back(userTimeInfo);

                    return NOP;
                }
            };

            class TCleanupVisitor : public kyotocabinet::DB::Visitor {
            public:

                TCleanupVisitor(const TInstant minTimeToKeep)
                    : NumRemoved(0)
                    , MinTimeToKeep(minTimeToKeep)
                {
                }

                size_t GetNumRemoved() const {
                    return NumRemoved;
                }

            private:
                size_t NumRemoved;
                TInstant MinTimeToKeep;

                const char* visit_full(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz, size_t*) override {
                    if (TStringBuf(kbuf, ksiz).StartsWith(SPECIAL_KEYS_PREFIX))
                        return NOP;

                    TMemoryInput mi(vbuf, vsiz);
                    ui32 FactorsVersion;
                    TInstant TimeStamp;
                    ::Load(&mi, FactorsVersion);
                    ::Load(&mi, TimeStamp);

                    if (TimeStamp < MinTimeToKeep) {
                        ++NumRemoved;
                        return REMOVE;
                    }

                    return NOP;
                }
            };

            if (!UseBdb)
                return;

            if (DbSize < ANTIROBOT_DAEMON_CONFIG.MaxDbSize)
                return;

            AtomicSet(IsCleaning, 1);

            size_t oldDbSize = DbSize;
            TWriteGuard guard(DbCleanupMutex);
            size_t toDelete = DbSize - ANTIROBOT_DAEMON_CONFIG.MaxDbSize + ANTIROBOT_DAEMON_CONFIG.DbUsersToDelete;

            UserTimes.clear();
            UserTimes.reserve(DbSize * 1.1);

            TGetTimeVisitor getTimeVisitor(UserTimes);

            TInstant startTime = TInstant::Now();
            Db_->iterate(&getTimeVisitor, false);
            TInstant endGetTimesTime = TInstant::Now();

            std::sort(UserTimes.begin(), UserTimes.end());
            TInstant minTimeToKeep = UserTimes[toDelete].TimeStamp;

            TCleanupVisitor cleanupVisitor(minTimeToKeep);

            TInstant startCleanupTime = TInstant::Now();
            Db_->iterate(&cleanupVisitor, false);
            TInstant endCleanupTime = TInstant::Now();

            TString dumpFile = DbDir + "/" + DATABASE_NAME + "_dump";

            Sync();

            {
                std::ofstream f(dumpFile.data());
                Db_->dump_snapshot(&f);
            }

            TInstant startLoadTime = TInstant::Now();

            OpenFreshDb();
            {
                std::ifstream f (dumpFile.data());
                Db_->load_snapshot(&f);
            }
            TInstant endTime = TInstant::Now();

            Sync();

            DbSize = Db_->count();

            EVLOG_MSG << EVLOG_ERROR << "Cleanup db, old size: " << oldDbSize << ", new size: " << DbSize
                << ", removed: " << cleanupVisitor.GetNumRemoved()
                << ", collect timestamps time: " << (endGetTimesTime - startTime)
                << ", cleanup time: " << (endCleanupTime - startCleanupTime)
                << ", dump time: " << (startLoadTime - endCleanupTime)
                << ", load time: " << (endTime - startLoadTime)
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
                int32_t valueSize = Db_->get(DB_VERSION_KEY.data(), DB_VERSION_KEY.size(), valueBuf.Data(), valueBuf.Size());
                if (valueSize < 0 || TStringBuf(valueBuf.Data(), valueSize) != CUR_DB_VERSION) {
                    EVLOG_MSG << EVLOG_ERROR << "Incorrect db version, creating new db";
                    return false;
                }
            } else {
                if (!Db_->set(DB_VERSION_KEY.data(), DB_VERSION_KEY.size(), CUR_DB_VERSION.data(), CUR_DB_VERSION.size()))
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
                NFs::Remove(baseFile);
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
                Db_.Reset();
            }

            NFs::Rename(baseFile, bakFile);
            EVLOG_MSG << EVLOG_ERROR << "Switch to fresh database";
            return TryOpenBase(baseFile);
        }

        bool TryReadFromDb(const TUid& uid) {
            TPerUidHashData& uidData = GetPerUidHashData(uid);
            uidData.ThreatLevelDummy.Clear();
            uidData.ThreatLevelPtr = &uidData.ThreatLevelDummy;

            TTempBuf valueBuf;
            int32_t valueSize = -1;

            if (DbCleanupMutex.TryAcquireRead()) {
                try {
                    valueSize = Db_->get(reinterpret_cast<const char*>(&uid), sizeof(TUid), valueBuf.Data(), valueBuf.Size());
                } catch (...) {
                    AtomicIncrement(DbGetExceptions);
                }
                DbCleanupMutex.ReleaseRead();
            } else {
                AtomicIncrement(SkippedReads);
                return false;
            }

            if (valueSize == -1) {
                AtomicIncrement(NumUidNotInBase);
                return false;
            }
            AtomicIncrement(NumUidInBase);

            TMemoryInput mi(valueBuf.Data(), valueSize);
            ::Load(&mi, uidData.ThreatLevelDummy);
            return true;
        }

        void WriteData(const TUid& uid, const TThreatLevel& threatLevel) {
            TTempBufOutput out;
            try {
                ::Save(&out, threatLevel);
            } catch(...) {
                ythrow yexception() << "buf error in WriteData: " << CurrentExceptionMessage();
            }

            // TODO: Figure out the problem here.
            NSan::Unpoison(out.Data(), out.Filled());

            if (!Db_->set(reinterpret_cast<const char*>(&uid), sizeof(TUid), out.Data(), out.Filled())) {
                ++NumWriteErrors;
                ++NumWriteErrorsSinceLastDbReset;
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

            if (UseBdb) {
                DirtyItems.Add(uid);
            }
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

        void FlushDirtyItems(size_t maxItemsToFlush) {
            TInstant startTime = TInstant::Now();

            LastFlushDirtySetSize = DirtyItems.Size();
            TUidVector dirtyList;
            DirtyItems.MoveAll(dirtyList);

            {
                TReadGuard cacheGuard(CacheMutex);

                TlTimes.clear();
                TlTimes.reserve(UidDataList.size());

                for (TUidVector::const_iterator toDirty = dirtyList.begin(); toDirty != dirtyList.end(); ++toDirty) {
                    TUidDataList::iterator toTl = UidDataList.find(*toDirty);
                    Y_ASSERT(toTl != UidDataList.end());

                    if (toTl != UidDataList.end()) {
                        TTlTimeInfo tlTimeInfo;
                        tlTimeInfo.TimeStamp = toTl->second->GetLastFlushTime().GetValue();
                        tlTimeInfo.AggregationLevel = toTl->first.GetAggregationLevel();
                        tlTimeInfo.ToTl = toTl;
                        TlTimes.push_back(tlTimeInfo);
                    }
                }
            }

            if (TlTimes.size() > maxItemsToFlush) {
                std::sort(TlTimes.begin(), TlTimes.end(), TTlTimeInfo::LessForFlushDirty);
                TlTimes.erase(TlTimes.begin() + maxItemsToFlush, TlTimes.end());
            }

            for (TTlTimeInfoVector::const_iterator it = TlTimes.begin(); it != TlTimes.end(); ++it) {
                it->ToTl->second->SetFlushed(startTime);
                WriteData(it->ToTl->first, *it->ToTl->second);
            }

            Sync();

            DbSize = Db_->count();

            LastFlushTime = TInstant::Now();
            LastFlushDuration = LastFlushTime - startTime;
            LastFlushNumItems = TlTimes.size();

            EVLOG_MSG << "FlushDirtyItems:"
                << " items: " << LastFlushNumItems
                << ", time: " << LastFlushDuration
                << ", avrg: " << TDuration::MicroSeconds(LastFlushDuration.MicroSeconds() / Max<ui64>(LastFlushNumItems, 1))
                << ", dirty set: " << LastFlushDirtySetSize
                << ", cache: " << UidDataList.size();

            if (ANTIROBOT_DAEMON_CONFIG.NumWriteErrorsToResetDb > 0 && NumWriteErrorsSinceLastDbReset >= ANTIROBOT_DAEMON_CONFIG.NumWriteErrorsToResetDb) {
                EVLOG_MSG << EVLOG_ERROR << "Too many db write errors, resetting database";
                NumWriteErrorsSinceLastDbReset = 0;
                OpenFreshDb();
            }
        }

        void ClearCache() {
            if (UidDataList.size() < ANTIROBOT_DAEMON_CONFIG.MaxSafeUsers)
                return;

            if (UidDataList.size() < DirtyItems.Size() + ANTIROBOT_DAEMON_CONFIG.UsersToDelete)
                return;

            TWriteGuard guard(CacheMutex);
            TInstant s = TInstant::Now();

            TlTimes.clear();
            TlTimes.reserve(UidDataList.size());

            for (TUidDataList::iterator toTl = UidDataList.begin(); toTl != UidDataList.end(); ++toTl) {
                Y_ASSERT(!!toTl->second);

                if (!DirtyItems.Has(toTl->first) && !UidsInUse.IsInUse(toTl->first)) {
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
                Cerr << "ClearCache() time: " << TDuration(TInstant::Now() - s) << "\tdeleted: " << deleteCount << " of " << TlTimes.size() << Endl;
            }

        }

        TPerUidHashData& GetPerUidHashData(const TUid& uid) {
            return PerUidHashData[uid.Id % NUM_UID_DATA];
        }

        friend class TSyncThread;
    };

    TUserBase::TUserBase(const TString& baseDir)
        : Impl(new TImpl(baseDir))
    {
    }

    TUserBase::~TUserBase() {
    }

    TUserBase::TValue TUserBase::Get(const TUid& uid) {
        return Impl->Get(uid);
    }

    void TUserBase::DeleteOldUsers() {
        Impl->DeleteOldUsers();
    }

    void TUserBase::PrintStats(TStatsWriter& out) const {
        Impl->PrintStats(out);
    }

    void TUserBase::PrintMemStats(IOutputStream& out) const {
        Impl->PrintMemStats(out);
    }

    void TUserBase::DumpCache(IOutputStream& out) const {
        Impl->DumpCache(out);
    }

    TUserBase::TValue::TValue(TImpl& userBase, const TUid& uid)
        : UserBase(userBase)
        , Uid(uid)
        , Ownership(true)
    {
        UserBase.AcquireValue(Uid);
    }

    TUserBase::TValue::TValue(const TValue& copy)
        : UserBase(copy.UserBase)
        , Uid(copy.Uid)
        , Ownership(true)
    {
        copy.Ownership = false;
    }

    TUserBase::TValue::~TValue() {
        if (Ownership) {
            UserBase.LeaveValue(Uid);
        }
    }

    TThreatLevel* TUserBase::TValue:: operator->() const {
        return UserBase.GetPerUidHashData(Uid).ThreatLevelPtr;
    }
}

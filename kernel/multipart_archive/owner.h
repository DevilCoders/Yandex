#pragma once

#include <kernel/multipart_archive/abstract/globals.h>
#include <kernel/multipart_archive/config/config.h>
#include <kernel/index_mapping/index_mapping.h>

#include <library/cpp/threading/named_lock/named_lock.h>
#include <library/cpp/logger/global/global.h>

#include <util/generic/singleton.h>
#include <util/system/rwlock.h>
#include <util/system/mutex.h>
#include <util/generic/hash.h>


namespace NRTYArchive {

    void HardLinkOrCopy(const TFsPath& from, const TFsPath& to);

    class TMappedFileGuard {
    public:
        using TPtr = TAtomicSharedPtr<TMappedFileGuard>;

        TMappedFileGuard(const TString& fileName)
            : FileName(fileName)
        {
            CHECK_WITH_LOG(fileName);
            TIndexPrefetchOptions opts;
            opts.TryLock = true;
            TIndexPrefetchResult prefetchRes = PrefetchMappedIndex(fileName, opts);
            CHECK_WITH_LOG(prefetchRes.Locked);
        }

        ~TMappedFileGuard() {
            ReleaseMappedIndexFile(FileName);
        }

    private:
        TString FileName;
    };


    template <class TMultipart>
    class TMultipartOwner {
        Y_DECLARE_SINGLETON_FRIEND();

        using TSelf = TMultipartOwner<TMultipart>;

    private:
        class TArchiveDelete {
        public:
            static inline void Destroy(TMultipart* archive) noexcept {
                TSelf::Release(archive->GetPath());
            }
        };

    public:
        TMultipartOwner() {
            EnableGlobalIndexFileMapping();
        }

        ~TMultipartOwner() {
            CHECK_WITH_LOG(Archives.empty());
        }

        using TPtr = TAtomicSharedPtr<TMultipart, TArchiveDelete>;

        template<typename ... TArgs>
        static TPtr Create(const TFsPath& path, const TMultipartConfig& config, TArgs... args) {
            return Instance().GetOrCreate(path, config, args...);
        }

        static TPtr Find(const TFsPath& path) {
            return Instance().GetMultipart(path, nullptr);
        }

        static void Repair(const TFsPath& path, const TMultipartConfig& config) {
            Instance().DoRepair(path, config);
        }

        static bool Exists(const TFsPath& path) {
            return GetFatPath(path).Exists();
        }

        static void Remove(const TFsPath& path) {
            Instance().DoRemove(path);
        }

        static void Rename(const TFsPath& from, const TFsPath& to) {
            Instance().DoRename(from, to);
        }

        static bool Check(const TFsPath& path) {
            if (!Exists(path))
                return false;
            return Instance().DoCheck(path);
        }

        static bool Empty(const TFsPath& path) {
            return Instance().CheckEmpty(path);
        }

        static TString CreateKey(const TFsPath& path) {
            TFsPath newPath = path;

            if (!newPath.IsAbsolute())
                newPath = path.Cwd() / path;

            newPath.Fix();

            return newPath.GetPath();
        }

    private:
        static TSelf& Instance() {
            return *Singleton<TSelf>();
        }

        template<typename ... TArgs>
        TPtr GetOrCreate(const TFsPath& path, const TMultipartConfig& config, TArgs... args) {
            TString key = CreateKey(path);
            IArchivePart::TConstructContext context = config.CreateReadContext();
            while (true) {
                TPtr archive = GetMultipart(path, &context);
                if (archive) {
                    return archive;
                }
                auto lock = NNamedLock::TryAcquireLock(key);
                if (lock) {
                    return Instance().DoCreate(path, config, lock, args...);
                }
            }
            FAIL_LOG("It's impossible to come here");
            return nullptr;
        }

        template<typename ... TArgs>
        TPtr DoCreate(const TFsPath& path, const TMultipartConfig& config, NNamedLock::TNamedLockPtr archiveLock, TArgs... args) {
            TString key = CreateKey(path);
            INFO_LOG << "Create archive " << key << Endl;
            TMultipart* obj = TMultipart::Construct(key, config, archiveLock, args...);
            TMappedFileGuard::TPtr mappingGuard(config.LockFAT ? new TMappedFileGuard(GetFatPath(key)) : nullptr);

            TGuard<TMutex> g(Lock);
            CHECK_WITH_LOG(Archives.insert(std::make_pair(key, new TRecord(obj, config.CreateReadContext(), mappingGuard))).second);
            CHECK_WITH_LOG(Archives.find(key) != Archives.end());
            return obj;
        }

        bool CheckEmpty(const TFsPath& path) {
            TPtr arc = GetMultipart(path, nullptr);
            if (!!arc)
                return arc->GetDocsCount(false) == 0;

            return TMultipart::IsEmpty(path);
        }

        static void Release(const TFsPath& path) {
            Instance().DoRelease(path);
        }

        void DoRelease(const TFsPath& path) {
            INFO_LOG << "Release archive " << path << "/" << Archives.size() << Endl;
            TString key = CreateKey(path);

            TGuard<TMutex> g(Lock);
            auto it = Archives.find(key);
            CHECK_WITH_LOG(it != Archives.end());

            typename TRecord::TPtr rec;
            if (it->second->Release()) {
                INFO_LOG << "Remove archive " << path << Endl;
                rec = it->second;
                Archives.erase(it);
            }

            g.Release();
            if (rec) {
                INFO_LOG << "Destroy archive " << path << Endl;
                rec.Drop();
            }
        }

        void DoRemove(const TFsPath& path) {
            VERIFY_WITH_LOG(NNamedLock::TryAcquireLock(CreateKey(path)), "Trying to remove opened multipart");
            TMultipart::Remove(path);
        }

        void DoRename(const TFsPath& from, const TFsPath& to) {
            VERIFY_WITH_LOG(NNamedLock::TryAcquireLock(CreateKey(from)), "Trying to rename opened multipart");
            TMultipart::Remove(from);
            TMultipart::Rename(from, to);
        }

        bool DoCheck(const TFsPath& path) {
            if (!NNamedLock::TryAcquireLock(CreateKey(path)))
                return true;

            return TMultipart::AllRight(path);
        }

        void DoRepair(const TFsPath& path, const TMultipartConfig& config) {
            VERIFY_WITH_LOG(NNamedLock::TryAcquireLock(CreateKey(path)), "Can't repair opened multipart");
            TMultipart::Repair(path, config.CreateReadContext());
        }

        TPtr GetMultipart(const TFsPath& path, const IArchivePart::TConstructContext* context) {
            TString key = CreateKey(path);

            TGuard<TMutex> g(Lock);
            auto it = Archives.find(key);
            if (it == Archives.end()) {
                return nullptr;
            }
            if (context) {
                VERIFY_WITH_LOG(it->second->CheckContext(*context), "Readable context changed for %s: %s", path.GetPath().data(), context->ToString().data());
            }
            return it->second->Get();
        }

    private:
        struct TRecord {
        public:
            using TPtr = TAtomicSharedPtr<TRecord>;

            TRecord(TMultipart* multipart, const IArchivePart::TConstructContext& context, TMappedFileGuard::TPtr mappingGuard)
                : Archive(multipart)
                , MappingGuard(mappingGuard)
                , Context(context)
                , Counter(1) {}

            TMultipart* Get() {
                ++(Counter);
                return Archive.Get();
            }

            bool Release() {
                --(Counter);
                return Counter == 0;
            }

            bool CheckContext(const IArchivePart::TConstructContext& context) {
                return Context.ToString() == context.ToString();
            }

            ~TRecord() {
                CHECK_WITH_LOG(Counter == 0);
            }

        private:
            THolder<TMultipart> Archive;
            TMappedFileGuard::TPtr MappingGuard;
            IArchivePart::TConstructContext Context;
            ui32 Counter;
        };

        TMutex Lock;
        TMap<TString, typename TRecord::TPtr> Archives;
    };
}

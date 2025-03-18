#pragma once

#include "file_system_objects.h"
#include "fuse_config.h"

#include <aapi/lib/common/object_types.h>

#include <util/memory/segmented_string_pool.h>
#include <util/memory/pool.h>

namespace NAapi {

    const ui64 RPCS = 2;

    class TFileSystemHolder {
    public:
        TFileSystemHolder(const TString& discStore, const TString& proxyAddr);

        void AddRoot(const TPathInfo& path, const TString& commitInfo);

        int Getattr(const char* path, struct stat *st);
        int Readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
        int Open(const char *path, struct fuse_file_info *fi);
        int Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
        int Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
        int Release(const char *path, struct fuse_file_info *fi);
        int Readlink(const char *path, char *buf, size_t size);
        int Chmod(const char *path, mode_t mode);
        int Chown(const char *path, uid_t uid, gid_t gid);
        int Utimens(const char *path, const struct timespec tv[2]);
        int Unlink(const char *path);
        int Mknod(const char *path, mode_t mode, dev_t dev);
        int Mkdir(const char *path, mode_t mode);
        int Rmdir(const char *path);
        int Symlink(const char *to, const char *path);
        int Rename(const char *path, const char *to);
        int Link(const char *to, const char *path);
        int Truncate(const char *path, off_t size);
        void Destroy();

    private:
        struct TRootParams {
            TRootParams() = default;
            TRootParams(const TString rootHash, const TString commitInfo, const TString commitHash, bool usePrefetchHeuristics);

            TString RootHash;
            TString CommitInfo;
            TString CommitHash;
            bool UsePrefetchHeuristics;
        };

        class TImpl {
        public:
            using TFindCallback = std::function<int(TDirNode*, TDirEntry*)>;

            TImpl(const TString& discStore, const TString& proxyAddr);
            ~TImpl();

            void AddRoot(const TPathInfo& path, const TString& commitInfo);

            int Getattr(const char* path, struct stat *st);
            int Readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
            int Open(const char *path, struct fuse_file_info *fi);
            int Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
            int Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
            int Release(const char *path, struct fuse_file_info *fi);
            int Readlink(const char *path, char *buf, size_t size);
            int Chmod(const char *path, mode_t mode);
            int Chown(const char *, uid_t, gid_t);
            int Utimens(const char *path, const struct timespec tv[2]);
            int Unlink(const char *path);
            int Mknod(const char *path, mode_t mode, dev_t dev);
            int Mkdir(const char *path, mode_t mode);
            int Rmdir(const char *path);
            int Symlink(const char *to, const char *path);
            int Rename(const char *path, const char *to);
            int Link(const char *to, const char *path);
            int Truncate(const char *path, off_t off);

        private:
            bool HasChild(TDirNode* d, const TStringBuf& name, TDirEntry** child = nullptr);
            bool Remove(TDirNode* d, TDirEntry* e);
            bool InsertOne(TDirNode* d, TDirEntry* e);
            void RequestDirEntries(TDirNode* dir);
            void InitFsTree(const TPathInfo& path);
            int FindNoLock(const TStringBuf& pathStr, TDirNode** dir, TDirEntry** entry, bool enterLastDir);
            int Find(const TStringBuf& pathStr, TFindCallback cb, bool enterLastDir, bool writeLock);
            ui32 NewDirInode();

        private:
            TAtomic DirsCount = 0;

            segmented_string_pool Strings;
            TMemoryPool Objects;

            TDirNode* FakeRoot;
            TDirNode* Root;

            TRootParams RootParams;

            TString ProxyAddr;
            THolder<NStore::TDiscStore> DiscStore;

            THolder<TFiles<RPCS>> Files;
        };

    private:
        THolder<TImpl> Fs;
    };

}

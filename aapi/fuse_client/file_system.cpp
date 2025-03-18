#include "file_system.h"
#include "utils.h"

#include <library/cpp/threading/future/future.h>

#include <aapi/lib/common/async_reader.h>
#include <aapi/lib/common/git_hash.h>
#include <aapi/lib/store/disc_store.h>
#include <aapi/lib/node/tree.fbs.h>

#include <util/string/builder.h>


namespace NAapi {

using NThreading::TFuture;
using NThreading::TPromise;
using NThreading::NewPromise;

// ============== TFileSystemHolder::TRootParams ==============

TFileSystemHolder::TRootParams::TRootParams(const TString rootHash, const TString commitInfo, const TString commitHash, bool usePrefetchHeuristics)
    : RootHash(rootHash)
    , CommitInfo(commitInfo)
    , CommitHash(commitHash)
    , UsePrefetchHeuristics(usePrefetchHeuristics)
{
}

// ============== TFileSystemHolder::TImpl ==============

TFileSystemHolder::TImpl::TImpl(const TString& discStore, const TString& proxyAddr)
    : Objects(64<<10, TMemoryPool::TLinearGrow::Instance())
    , ProxyAddr(proxyAddr)
    , DiscStore(new NStore::TDiscStore(discStore))
    , Files(new TFiles<RPCS>(DiscStore.Get(), proxyAddr))
{
}

TFileSystemHolder::TImpl::~TImpl() {
    Files.Destroy();
}

void TFileSystemHolder::TImpl::AddRoot(const TPathInfo& path, const TString& commitInfo) {
    RootParams = TRootParams(path.Hash, commitInfo, GitLikeHash(commitInfo), !AsciiHasSuffixIgnoreCase(path.Path, "/arcadia_tests_data") && !AsciiHasSuffixIgnoreCase(path.Path, "/data"));
    Cerr << "Use prefetch heuristics: " << (int)RootParams.UsePrefetchHeuristics << Endl;
    InitFsTree(path);
}

int TFileSystemHolder::TImpl::Getattr(const char* path, struct stat *st) {
    TPromise<std::pair<TFileStat, ui64> > p = NewPromise<std::pair<TFileStat, ui64> >();

    int find = Find(path, [this, p](TDirNode*, TDirEntry* e) mutable {
        if (e->IsDir()) {
            p.SetValue(std::make_pair(e->DirNode->Stat, e->Inode));

        } else {
            Files->Getattr(e->Inode).Subscribe([p, inode=e->Inode](TFuture<TFileStat> f) mutable {
                try {
                    p.SetValue(std::make_pair(f.GetValueSync(), inode));
                } catch (yexception ex) {
                    p.SetException(ex.what());
                }
            });
        }

        return 0;
    }, false, false);

    if (find < 0) {
        return find;
    }

    TFileStat stat;
    ui64 inode;
    std::tie(stat, inode) = p.GetFuture().GetValueSync();  // throws

    Zero(*st);
    st->st_mode  =   stat.Mode;
    st->st_uid   =    stat.Uid;
    st->st_gid   =    stat.Gid;
    st->st_nlink = stat.NLinks;
    st->st_size  =   stat.Size;
    st->st_mtime =  stat.MTime;
    st->st_ino = inode;

    return find;
}

int TFileSystemHolder::TImpl::Readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    Y_UNUSED(offset);
    Y_UNUSED(fi);

    return Find(path, [buf, filler](TDirNode* d, TDirEntry* e) {
        Y_ENSURE(e == nullptr);

        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);

        TDirEntry* de = d->Entries;

        while (de) {
            filler(buf, de->Name.data(), NULL, 0);
            de = de->NextEntry;
        }

        return 0;
    }, true, false);
}

int TFileSystemHolder::TImpl::Open(const char *path, struct fuse_file_info *fi) {
    int find = Find(path, [](TDirNode*, TDirEntry* e) -> int {
        if (e->IsDir()) {
            return -EISDIR;
        }

        return e->Inode;
    }, false, false);

    if (find < 0) {
        return find;
    }

    Files->RequestFile(find).Wait();
    fi->fh = Files->Open(find).GetValueSync();
    return 0;
}

int TFileSystemHolder::TImpl::Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    Y_UNUSED(path);
    Y_ENSURE(fi->fh >= 0);
    return static_cast<int>(Files->Read(fi->fh, buf, size, offset));
}

int TFileSystemHolder::TImpl::Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    Y_UNUSED(path);
    Y_ENSURE(fi->fh >= 0);
    return static_cast<int>(Files->Write(fi->fh, buf, size, offset));
}

int TFileSystemHolder::TImpl::Release(const char *path, struct fuse_file_info *fi) {
    Y_UNUSED(path);
    Y_ENSURE(fi->fh >= 0);
    Files->Close(fi->fh);
    return 0;
}

int TFileSystemHolder::TImpl::Readlink(const char *path, char *buf, size_t size) {
    int find = Find(path, [](TDirNode*, TDirEntry* e) -> int {
        if (e->IsDir()) {
            return -EISDIR;
        }

        return e->Inode;
    }, false, false);

    if (find < 0) {
        return find;
    }

    Files->RequestFile(find).Wait();
    return Files->Readlink(find, buf, size).GetValueSync();
}

int TFileSystemHolder::TImpl::Chmod(const char *path, mode_t mode) {
    auto p = NewPromise<int>();

    int find = Find(path, [this, mode, p](TDirNode*, TDirEntry* e) mutable -> int {
        if (e->IsDir()) {
            p.SetValue(e->DirNode->Chmod(mode));
        } else {
            Files->Chmod(e->Inode, mode).Subscribe([p](TFuture<int> f) mutable {
                try {
                    p.SetValue(f.GetValueSync());
                } catch (yexception e) {
                    p.SetException(e.what());
                }
            });
        }

        return 0;
    }, false, true);

    if (find < 0) {
        return find;
    }

    return p.GetFuture().GetValueSync();
}

int TFileSystemHolder::TImpl::Chown(const char *, uid_t, gid_t) {
    return -EPERM;
}

int TFileSystemHolder::TImpl::Utimens(const char *path, const struct timespec tv[2]) {
    auto p = NewPromise<void>();

    int find = Find(path, [this, tv, p](TDirNode*, TDirEntry* e) mutable -> int {
        if (e->IsDir()) {
            e->DirNode->Utimens(tv);
            p.SetValue();
        } else {
            Files->Utimens(e->Inode, tv).Subscribe([p](TFuture<void> f) mutable {
                try {
                    f.GetValueSync();
                    p.SetValue();
                } catch (yexception e) {
                    p.SetException(e.what());
                }
            });
        }

        return 0;
    }, false, true);

    if (find < 0) {
        return find;
    }

    p.GetFuture().GetValueSync();
    return 0;
}

int TFileSystemHolder::TImpl::Unlink(const char *path) {
    return Find(path, [this](TDirNode* d, TDirEntry* e) -> int {
        if (e->IsDir()) {
            return -EISDIR;
        }

        Y_ENSURE(Remove(d, e));
        Files->Unlink(e->Inode).Wait();

        return 0;
    }, false, true);
}

int TFileSystemHolder::TImpl::Mknod(const char *path, mode_t mode, dev_t dev) {
    if (!(mode & S_IFREG)) {
        return -EPERM;
    }

    TFsPath p(path);
    const TString parent = p.Parent().GetPath();
    const TString name = p.Basename();

    return Find(parent, [this, name, mode, dev](TDirNode* parentNode, TDirEntry* e) -> int {
        Y_ENSURE(e == nullptr);

        if (HasChild(parentNode, name)) {
            return -EEXIST;
        }

        TDirEntry* ent = new (Objects) TDirEntry;
        ent->Name = Strings.append(name.data());
        ent->Inode = Files->AddInode(CustomModeStats(mode, Now().TimeT(), 0));

        Y_ENSURE(InsertOne(parentNode, ent));

        Y_UNUSED(dev);
        return 0;
    }, true, true);
}

int TFileSystemHolder::TImpl::Mkdir(const char *path, mode_t mode) {
    TFsPath p(path);
    const TString parent = p.Parent().GetPath();
    const TString name = p.Basename();

    return Find(parent, [this, name, mode](TDirNode* parentNode, TDirEntry* e) -> int {
        Y_ENSURE(e == nullptr);

        if (HasChild(parentNode, name)) {
            return -EEXIST;
        }

        TDirEntry* ent = new (Objects) TDirEntry;
        ent->Name = Strings.append(name.data());
        ent->DirNode = new (Objects) TDirNode(CustomModeStats(mode | S_IFDIR, Now().TimeT(), 10));
        ent->Inode = NewDirInode();

        Y_ENSURE(InsertOne(parentNode, ent));

        return 0;

    }, true, true);
}

int TFileSystemHolder::TImpl::Rmdir(const char *path) {
    return Find(path, [this](TDirNode* d, TDirEntry* e) -> int {
        if (!e->IsDir()) {
            return -ENOTDIR;
        }

        if (e->DirNode->Entries) {
            return -ENOTEMPTY;
        }

        Y_ENSURE(Remove(d, e));

        return 0;
    }, false, true);
}

int TFileSystemHolder::TImpl::Symlink(const char *to, const char *path) {
    TFsPath p(path);
    const TString parent = p.Parent().GetPath();
    const TString name = p.Basename();
    TString data(to);
    TString hash = GitLikeHash(data);

    return Find(parent, [this, name, hash, data](TDirNode* parentNode, TDirEntry* e) -> int {
        Y_ENSURE(e == nullptr);

        if (HasChild(parentNode, name)) {
            return -EEXIST;
        }

        TDirEntry* ent = new (Objects) TDirEntry;
        ent->Name = Strings.append(name.data());
        ent->Inode = Files->AddInodeContent(
            TStringBuf(Strings.append(hash.data(), hash.size()), hash.size()),
            Strings.append(data.data()),
            DefaultModeStats(EEntryMode::EEM_LINK, Now().TimeT(), data.size())
        );

        Y_ENSURE(InsertOne(parentNode, ent));

        return 0;

    }, true, true);
}

int TFileSystemHolder::TImpl::Rename(const char *path, const char *to) {
    TFsPath toPath(to);
    const TString toParent = toPath.Parent().GetPath();
    const TString toName = toPath.Basename();

    TWriteGuard g(FakeRoot->Lock);  // Global filesystem lock, TODO

    TDirNode* pathParentNode;
    TDirEntry* pathEntry;
    int find = FindNoLock(path, &pathParentNode, &pathEntry, false);
    if (find < 0) {
        return find;
    }

    TDirNode* toParentNode;
    TDirEntry* e;
    int find2 = FindNoLock(toParent, &toParentNode, &e, true);
    if (find2 < 0) {
        return find2;
    }
    Y_ENSURE(e == nullptr);

    TDirEntry *toEntry;
    if (HasChild(toParentNode, toName, &toEntry)) {
        if (toEntry == pathEntry) {
            Y_ENSURE(pathParentNode == toParentNode);
            return 0;
        }

        if (!pathEntry->IsDir() && toEntry->IsDir()) {
            return -EISDIR;
        }

        if (pathEntry->IsDir() && !toEntry->IsDir()) {
            return -ENOTDIR;
        }

        Y_ENSURE(Remove(toParentNode, toEntry));
    }

    Y_ENSURE(Remove(pathParentNode, pathEntry));
    pathEntry->Name = Strings.append(toName.data());
    Y_ENSURE(InsertOne(toParentNode, pathEntry));
    return 0;
}

int TFileSystemHolder::TImpl::Link(const char *to, const char *path) {
    Y_UNUSED(to);
    Y_UNUSED(path);
    return -ENOTSUP;  // TODO: need good inodes for correct link opperation
//        int find = Find(to, [](TDirNode*, TDirEntry* e) -> int {
//            if (e->IsDir()) {
//                return -EISDIR;
//            }
//
//            return e->Inode;
//        }, false, false);
//
//        if (find < 0) {
//            return find;
//        }
//
//        TFsPath p(path);
//        const TString parent = p.Parent().GetPath();
//        const TString name = p.Basename();
//
//        return Find(parent, [this, inode = find, name](TDirNode* parentDir, TDirEntry* e) -> int {
//            Y_ENSURE(e == nullptr);
//
//            if (HasChild(parentDir, name)) {
//                return -EEXIST;
//            }
//
//            TDirEntry* ent = new (Objects) TDirEntry;
//            ent->Name = Strings.append(~name);
//            ent->Inode = inode;
//
//            Y_ENSURE(InsertOne(parentDir, ent));
//
//            Files->Link(inode).Wait();
//
//            return 0;
//        }, true, true);
}

int TFileSystemHolder::TImpl::Truncate(const char *path, off_t off) {
    return Find(path, [this, off](TDirNode*, TDirEntry* e) -> int {
        if (e->IsDir()) {
            return -EISDIR;
        }

        return Files->Truncate(e->Inode, off).GetValueSync();
    }, false, false);
}

bool TFileSystemHolder::TImpl::HasChild(TDirNode* d, const TStringBuf& name, TDirEntry** child) {
    TDirEntry* e = d->Entries;

    while (e && e->Name != name) {
        e = e->NextEntry;
    }

    if (e) {
        if (!!child) {
            (*child) = e;
        }
        return true;
    }

    return false;
}

bool TFileSystemHolder::TImpl::Remove(TDirNode* d, TDirEntry* e) {
    if (d->Entries == e) {
        d->Entries = e->NextEntry;
        return true;
    }

    TDirEntry* p = d->Entries;
    while (p && p->NextEntry != e) {
        p = p->NextEntry;
    }

    if (!p) {
        return false;
    }

    p->NextEntry = e->NextEntry;
    return true;
}

bool TFileSystemHolder::TImpl::InsertOne(TDirNode* d, TDirEntry* e) {
    TDirEntry* de = d->Entries;

    while (de && de->Name != e->Name) {
        de = de->NextEntry;
    }

    if (de) {
        return false;
    }

    e->NextEntry = d->Entries;
    d->Entries = e;
    return true;
}

void TFileSystemHolder::TImpl::RequestDirEntries(TDirNode* dir) {
    if (AtomicCas(&dir->Requested, true, false)) {
        TWriteGuard guard(dir->Lock);
        TDirEntry* e = dir->Entries;
        while (e) {
            if (!e->IsDir()) {
                Files->RequestFile(e->Inode);
            }
            e = e->NextEntry;
        }
    }
}

void TFileSystemHolder::TImpl::InitFsTree(const TPathInfo& path) {
    using TWalkReader = grpc::ClientReader<TDirectories>;

    grpc::ClientContext ctx;
    THash hash;
    hash.SetHash(RootParams.RootHash);
    THolder<NVcs::Vcs::Stub> stub = THolder<NVcs::Vcs::Stub>(CreateNewStub(ProxyAddr));
    THolder<TWalkReader> stream = THolder<TWalkReader>(stub->Walk(&ctx, hash).release());
    TAsyncReader<TWalkReader, TDirectories> asyncStream(stream.Get(), 0);
    const bool isRoot = (TFsPath(path.Path).Fix() == "/");
    time_t start = Now().TimeT();

    THashMap<TString, TDirNode*> PathToNode;

    TDirectories dirsChunk;
    while (asyncStream.Read(dirsChunk)) {
        for (const TDirectory& dir: dirsChunk.GetDirectories()) {
            auto tree = NNode::GetTree(dir.GetBlob().data());

            const TString& dirPath = dir.GetPath();
            TDirNode* dirNode;

            auto it = PathToNode.find(dirPath);
            if (it == PathToNode.end()) {
                dirNode = new (Objects) TDirNode(start, dir.GetBlob().size());
                PathToNode[dirPath] = dirNode;
            } else {
                dirNode = it->second;
                dirNode->Stat.Size = dir.GetBlob().size();
            }

            const bool isRootDir = dir.GetHash() == RootParams.RootHash;

            TDirEntry* e;

            if (isRootDir) {
                Root = dirNode;
                FakeRoot = new (Objects) TDirNode(start);

                FakeRoot->Entries = new (Objects) TDirEntry;
                FakeRoot->Entries->Name = Strings.append(dirPath.data());
                FakeRoot->Entries->DirNode = Root;
                FakeRoot->Entries->Inode = NewDirInode();

                Root->Entries = new (Objects) TDirEntry;
                Root->Entries->Name = Strings.append("__SVNVERSION__");
                Root->Entries->Inode = Files->AddInodeContent(RootParams.CommitHash, RootParams.CommitInfo, DefaultModeStats(EEntryMode::EEM_REG, start, RootParams.CommitInfo.size()));

                if (tree->entries()->size()) {
                    Root->Entries->NextEntry = new (Objects) TDirEntry;
                    e = Root->Entries->NextEntry;
                }
            } else if (tree->entries()->size()) {
                dirNode->Entries = new (Objects) TDirEntry;
                e = dirNode->Entries;
            }

            for (ui64 i = 0, size = tree->entries()->size(); i < size; ++i) {
                const auto ent = tree->entries()->Get(i);
                const TStringBuf name(ent->name()->data(), ent->name()->size());

                if (isRoot && (static_cast<EEntryMode>(ent->mode()) == EEntryMode::EEM_DIR && dir.GetHash() == RootParams.RootHash)) {
                    if (AsciiHasPrefix(name, "secure")) {
                        continue;
                    }
                }

                const TString entryPath = TStringBuilder{} << dirPath << "/" << name;

                e->Name = TStringBuf(Strings.append(name.data(), name.size()));
                const EEntryMode mode = static_cast<EEntryMode>(ent->mode());

                if (i + 1 < size) {
                    e->NextEntry = new (Objects) TDirEntry;
                } else {
                    e->NextEntry = nullptr;
                }

                if (mode == EEntryMode::EEM_DIR) {
                    e->DirNode = new (Objects) TDirNode(start);
                    e->Inode = NewDirInode();
                    PathToNode[entryPath] = e->DirNode;
                } else {
                    e->DirNode = nullptr;
                    const char* bp = Strings.append(reinterpret_cast<const char*>(ent->blobs()->data()), ent->blobs()->size());
                    e->Inode = Files->AddInodeBlobs(
                        TStringBuf(Strings.append(reinterpret_cast<const char*>(ent->hash()->data()), 20), 20),
                        TStringBuf(bp, ent->blobs()->size()),
                        DefaultModeStats(mode, start, ent->size())
                    );
                }

                e = e->NextEntry;
            }
        }
    }

    Files->Freeze();

    asyncStream.Join();
    Y_ENSURE(stream->Finish().ok());
    Y_ENSURE(Root);
    Y_ENSURE(FakeRoot);
    Root->Stat.ATime = Root->Stat.MTime = Root->Stat.CTime = 0;
}

int TFileSystemHolder::TImpl::FindNoLock(const TStringBuf& pathStr, TDirNode** dir, TDirEntry** entry, bool enterLastDir) {
    TFsPath path(pathStr.data());
    auto& split = path.PathSplit();

    if (split.size() == 0) {
        if (RootParams.UsePrefetchHeuristics) {
            RequestDirEntries(Root);
        }

        if (enterLastDir) {
            (*dir) = Root;
            (*entry) = nullptr;
        } else {
            (*dir) = FakeRoot;
            (*entry) = FakeRoot->Entries;
        }

        return 0;
    }

    TDirNode* cur = Root;

    for (ui64 partI = 0; partI + 1 < split.size(); ++partI) {
        const TStringBuf& part = split[partI];
        bool found = false;

        TDirEntry* e = cur->Entries;
        while (e) {
            if (part == e->Name) {
                if (!e->IsDir()) {
                    return -ENOENT;
                }

                cur = e->DirNode;
                found = true;
                break;
            }

            e = e->NextEntry;
        }

        if (!found) {
            return -ENOENT;
        }
    }

    TDirEntry* e = cur->Entries;
    while (e) {
        if (split.back() == e->Name) {
            if (!enterLastDir) {
                (*dir) = cur;
                (*entry) = e;
                return 0;
            } else {
                if (!e->IsDir()) {
                    return -ENOENT;
                } else {
                    if (RootParams.UsePrefetchHeuristics) {
                        RequestDirEntries(e->DirNode);
                    }
                    (*dir) = e->DirNode;
                    (*entry) = nullptr;
                    return 0;
                }
            }
        }
        e = e->NextEntry;
    }

    return -ENOENT;
}

int TFileSystemHolder::TImpl::Find(const TStringBuf& pathStr, TFindCallback cb, bool enterLastDir, bool writeLock) {
    TFsPath path(pathStr.data());
    auto& split = path.PathSplit();
    TVector<TReadGuard> readGuards;
    readGuards.reserve(split.size());
    TVector<TWriteGuard> writeGuards;

    if (split.size() == 0) {
        if (RootParams.UsePrefetchHeuristics) {
            RequestDirEntries(Root);
        }

        if (writeLock) {
            TWriteGuard g(FakeRoot->Lock);
            if (enterLastDir) {
                return cb(Root, nullptr);
            } else {
                return cb(FakeRoot, FakeRoot->Entries);
            }

        } else {
            TReadGuard g(FakeRoot->Lock);
            if (enterLastDir) {
                return cb(Root, nullptr);
            } else {
                return cb(FakeRoot, FakeRoot->Entries);
            }
        }
    }

    TDirNode* cur = Root;

    for (ui64 partI = 0; partI + 1 < split.size(); ++partI) {
        const TStringBuf& part = split[partI];
        bool found = false;

        readGuards.emplace_back(cur->Lock);

        TDirEntry* e = cur->Entries;
        while (e) {
            if (part == e->Name) {
                if (!e->IsDir()) {
                    return -ENOENT;
                }

                cur = e->DirNode;
                found = true;
                break;
            }

            e = e->NextEntry;
        }

        if (!found) {
            return -ENOENT;
        }
    }

    if (RootParams.UsePrefetchHeuristics) {
        RequestDirEntries(cur);
    }

    if (!enterLastDir) {
        if (writeLock) {
            writeGuards.emplace_back(cur->Lock);
        } else {
            readGuards.emplace_back(cur->Lock);
        }
    } else {
        readGuards.emplace_back(cur->Lock);
    }

    TDirEntry* e = cur->Entries;
    while (e) {
        if (split.back() == e->Name) {
            if (!enterLastDir) {
                return cb(cur, e);
            } else {
                if (!e->IsDir()) {
                    return -ENOENT;
                } else {
                    if (RootParams.UsePrefetchHeuristics) {
                        RequestDirEntries(e->DirNode);
                    }
                    if (writeLock) {
                        TWriteGuard g(e->DirNode->Lock);
                        return cb(e->DirNode, nullptr);
                    } else {
                        TReadGuard g(e->DirNode->Lock);
                        return cb(e->DirNode, nullptr);
                    }
                }
            }
        }
        e = e->NextEntry;
    }

    return -ENOENT;
}

ui32 TFileSystemHolder::TImpl::NewDirInode() {
    return 1000000000 + static_cast<ui32>(AtomicGetAndIncrement(DirsCount));
}

// ============== TFileSystemHolder ==============

TFileSystemHolder::TFileSystemHolder(const TString& discStore, const TString& proxyAddr) {
    Fs = MakeHolder<TImpl>(discStore, proxyAddr);
}

void TFileSystemHolder::AddRoot(const TPathInfo& path, const TString& commitInfo) {
    Fs->AddRoot(path, commitInfo);
}

int TFileSystemHolder::Getattr(const char* path, struct stat *st) {
    return Fs->Getattr(path, st);
}

int TFileSystemHolder::Readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    return Fs->Readdir(path, buf, filler, offset, fi);
}

int TFileSystemHolder::Open(const char *path, struct fuse_file_info *fi) {
    return Fs->Open(path, fi);
}

int TFileSystemHolder::Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    return Fs->Read(path, buf, size, offset, fi);
}

int TFileSystemHolder::Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    return Fs->Write(path, buf, size, offset, fi);
}

int TFileSystemHolder::Release(const char *path, struct fuse_file_info *fi) {
    return Fs->Release(path, fi);
}

int TFileSystemHolder::Readlink(const char *path, char *buf, size_t size) {
    return Fs->Readlink(path, buf, size);
}

int TFileSystemHolder::Chmod(const char *path, mode_t mode) {
    return Fs->Chmod(path, mode);
}

int TFileSystemHolder::Chown(const char *path, uid_t uid, gid_t gid) {
    return Fs->Chown(path, uid, gid);
}

int TFileSystemHolder::Utimens(const char *path, const struct timespec tv[2]) {
    return Fs->Utimens(path, tv);
}

int TFileSystemHolder::Unlink(const char *path) {
    return Fs->Unlink(path);
}

int TFileSystemHolder::Mknod(const char *path, mode_t mode, dev_t dev) {
    return Fs->Mknod(path, mode, dev);
}

int TFileSystemHolder::Mkdir(const char *path, mode_t mode) {
    return Fs->Mkdir(path, mode);
}

int TFileSystemHolder::Rmdir(const char *path) {
    return Fs->Rmdir(path);
}

int TFileSystemHolder::Symlink(const char *to, const char *path) {
    return Fs->Symlink(to, path);
}

int TFileSystemHolder::Rename(const char *path, const char *to) {
    return Fs->Rename(path, to);
}

int TFileSystemHolder::Link(const char *to, const char *path) {
    return Fs->Link(to, path);
}

int TFileSystemHolder::Truncate(const char *path, off_t size) {
    return Fs->Truncate(path, size);
}

void TFileSystemHolder::Destroy() {
    Fs.Destroy();
}

} // namespace NAapi

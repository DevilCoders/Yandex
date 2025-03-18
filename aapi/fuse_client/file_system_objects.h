#pragma once

#include "protocol.h"
#include "utils.h"

#include <library/cpp/blockcodecs/stream.h>
#include <library/cpp/threading/future/future.h>

#include <aapi/lib/store/disc_store.h>

#include <util/generic/hash.h>
#include <util/stream/file.h>
#include <util/memory/pool.h>

#include <util/system/rwlock.h>

namespace NAapi {

struct TDirNode;

struct TDirEntry: public TPoolable {
    bool IsDir() const;
    bool IsFile() const;

    TStringBuf Name;
    TDirEntry* NextEntry = nullptr;

    // For dirs
    TDirNode* DirNode = nullptr;
    // For files and dirs
    ui64 Inode;
};

struct TDirNode: public TPoolable {
    TDirNode(time_t start, ui64 size = 10);
    TDirNode(const TFileStat& st);

    int Chmod(mode_t mode);
    void Utimens(const struct timespec tv[2]);

    TRWMutex Lock;
    TDirEntry* Entries = nullptr;
    TFileStat Stat;
    TAtomic Requested = false;
};

template <class T>
struct TFastHash {
    ui64 operator() (const T& k) {
        ui64 h;
        memcpy(&h, k.data(), 8);
        return h;
    }
};

template <size_t N>
class TFiles {
public:
    TFiles(NStore::TDiscStore* discStore, const TString& proxyAddr)
        : DiscStore(discStore)
        , Worker(new TThreadPool(TThreadPool::TParams().SetBlocking(true).SetCatching(false)))
    {
        Worker->Start(1);
        Objects3 = MakeHolder<TObjectsRpcs<TFiles, N>>(proxyAddr, this, 30);
    }

    ~TFiles() {
        // TODO: do better
        AtomicSet(Stopped, true);
        Worker->Stop();
        Objects3.Destroy();
    }

    ui64 AddInodeContent(const TStringBuf& hash, const TStringBuf& content, TFileStat stat) {
        Y_ENSURE(content.size() == stat.Size);
        ui64 inode = static_cast<ui64>(AtomicGetAndAdd(InodesCount, 1));

        Worker->SafeAddFunc([this, inode, hash, content, stat = std::move(stat)]() {
            if (IsStopped()) {
                return;
            }

            if (inode >= Inodes.size()) {
                Inodes.resize(inode + 1);
            }

            Inodes[inode].ContentId = InsertContentReady(hash, content);
            Inodes[inode].Stat = stat;
        });

        return inode;
    }

    ui64 AddInodeBlobs(const TStringBuf& hash, const TStringBuf& blobs, TFileStat stat) {
        ui64 inode = static_cast<ui64>(AtomicGetAndAdd(InodesCount, 1));

        Worker->SafeAddFunc([this, inode, hash, blobs, stat = std::move(stat)]() {
            if (IsStopped()) {
                return;
            }

            Y_ENSURE(!Frozen);

            if (inode >= Inodes.size()) {
                Inodes.resize(inode + 1);
            }

            Inodes[inode].ContentId = InsertContentBlobs(hash, blobs);
            Inodes[inode].Stat = stat;
        });

        return inode;
    }

    ui64 AddInode(TFileStat stat) {
        ui64 inode = static_cast<ui64>(AtomicGetAndAdd(InodesCount, 1));

        Worker->SafeAddFunc([this, inode, stat = std::move(stat)]() {
            if (IsStopped()) {
                return;
            }

            if (inode >= Inodes.size()) {
                Inodes.resize(inode + 1);
            }

            Inodes[inode].Readonly = false;
            Inodes[inode].Path = DiscStore->TmpPath();
            Inodes[inode].Stat = stat;
            TFileOutput out(Inodes[inode].Path);
        });

        return inode;
    }

    void Freeze() {
        Worker->SafeAddFunc([this]() {
            Frozen = true;
        });
    }

    void SetBlobData(const TString& hash, const TBlob& data) {
        Worker->SafeAddFunc([this, hash, data]() {
            // TODO: remove when bug is located
            if (data.Empty()) {
                Cerr << "Server returned empty blob for " << HexEncode(hash) << Endl;
            }
        
            if (IsStopped()) {
                return;
            }

            Y_ENSURE(Frozen);

            auto it = BlobIdx.find(hash);
            Y_ENSURE(it != BlobIdx.end());

            TInnBlob& blob = Blobs[it->second];

            if (blob.Downloaded()) {
                // In vain
                return;
            }

            blob.Data = data;

            for (ui64 cId: blob.ContentIds) {
                while (Contents[cId].WriteNextBlob(&Blobs, DiscStore)) {
                }
            }
        });
    }

    NThreading::TFuture<void> RequestFile(const ui64 inode) {
        NThreading::TPromise<void> promise = NThreading::NewPromise();

        Worker->SafeAddFunc([this, inode, promise]() mutable {
            if (IsStopped()) {
                promise.SetException("Stopped");
                return;
            }

            Y_ENSURE(Frozen);

            TInode& node = Inodes[inode];

            if (!node.Readonly) {
                promise.SetValue();
                return;
            }

            TFileContent& content = Contents[node.ContentId];

            if (content.Content) {
                promise.SetValue();
                return;
            }

            if (content.ChecFs(DiscStore, node.Stat.Size)) {
                promise.SetValue();
                return;
            }

            if (content.Restored()) {
                promise.SetValue();
                return;
            }

            if (!content.Requested()) {
                TVector<TStringBuf> blobs;
                Blobs.reserve(content.BlobIds.size());
                for (ui64 b: content.BlobIds) {
                    blobs.push_back(Blobs[b].Hash);
                }
                Objects3->Write(content.RpcId(), blobs);
                content.Promise = MakeHolder<NThreading::TPromise<void>>(NThreading::NewPromise<void>());
            }

            content.Promise->GetFuture().Subscribe([promise](auto) mutable {
                promise.SetValue();
            });
        });

        return promise.GetFuture();
    }

    NThreading::TFuture<ui64> Open(const ui64 inode) {
        NThreading::TPromise<ui64> promise = NThreading::NewPromise<ui64>();

        Worker->SafeAddFunc([this, inode, promise]() mutable {
            if (IsStopped()) {
                promise.SetException("Stopped");
                return;
            }

            Y_ENSURE(Frozen);

            TInode& node = Inodes[inode];

            node.Open(this);

            promise.SetValue(inode);
        });

        return promise.GetFuture();
    }

    ui64 Read(ui64 fd, char* buf, size_t size, off_t offset) {
        TReadContext ctx = ReadCtx(fd).GetValueSync();

        if (ctx.Content) {
            TMemoryInput in(ctx.Content.data(), ctx.Content.size());
            in.Skip(offset);
            return in.Read(buf, size);
        }

        return ctx.File->Pread(buf, size, offset);
    }

    NThreading::TFuture<int> Chmod(ui64 inode, mode_t mode) {
        NThreading::TPromise<int> promise = NThreading::NewPromise<int>();

        Worker->SafeAddFunc([this, inode, mode, promise]() mutable {
            if (IsStopped()) {
                promise.SetException("Stopped");
                return;
            }

            promise.SetValue(Inodes[inode].Chmod(mode));
        });

        return promise.GetFuture();
    }

    NThreading::TFuture<void> Utimens(ui64 inode, const struct timespec tv[2]) {
        NThreading::TPromise<void> promise = NThreading::NewPromise<void>();

        Worker->SafeAddFunc([this, inode, tv, promise]() mutable {
            if (IsStopped()) {
                promise.SetException("Stopped");
                return;
            }

            Inodes[inode].Utimens(tv);

            promise.SetValue();
        });

        return promise.GetFuture();
    }

    NThreading::TFuture<void> Unlink(ui64 inode) {
        NThreading::TPromise<void> promise = NThreading::NewPromise<void>();

        Worker->SafeAddFunc([this, inode, promise]() mutable {
            if (IsStopped()) {
                promise.SetException("Stopped");
                return;
            }

            Inodes[inode].Unlink();

            promise.SetValue();
        });

        return promise.GetFuture();
    }

    NThreading::TFuture<void> Link(ui64 inode) {
        NThreading::TPromise<void> promise = NThreading::NewPromise<void>();

        Worker->SafeAddFunc([this, inode, promise]() mutable {
            if (IsStopped()) {
                promise.SetException("Stopped");
                return;
            }

            Inodes[inode].Link();

            promise.SetValue();
        });

        return promise.GetFuture();
    }

    NThreading::TFuture<int> Truncate(ui64 inode, off_t off) {
        NThreading::TPromise<int> promise = NThreading::NewPromise<int>();

        Worker->SafeAddFunc([this, inode, off, promise]() mutable {
            if (IsStopped()) {
                promise.SetException("Stopped");
                return;
            }

            if (Inodes[inode].Readonly) {
                Inodes[inode].MakeWritable(this);
            }

            promise.SetValue(Inodes[inode].Truncate(off));
        });

        return promise.GetFuture();
    }

    ui64 Write(ui64 fd, const char* buf, size_t size, off_t offset) {
        WriteCtx(fd).GetValueSync().File->Pwrite(buf, size, offset);
        return size;
    }

    NThreading::TFuture<TFileStat> Getattr(ui64 inode) {
        NThreading::TPromise<TFileStat> promise = NThreading::NewPromise<TFileStat>();

        Worker->SafeAddFunc([this, inode, promise]() mutable {
            if (IsStopped()) {
                promise.SetException("Stopped");
                return;
            }

            promise.SetValue(Inodes[inode].GetStat());
        });

        return promise.GetFuture();
    }

    NThreading::TFuture<int> Readlink(ui64 inode, char *buf, size_t size) {
        NThreading::TPromise<int> promise = NThreading::NewPromise<int>();

        Worker->SafeAddFunc([this, inode, buf, size, promise]() mutable {
            if (IsStopped()) {
                promise.SetException("Stopped");
                return;
            }
            promise.SetValue(Inodes[inode].Readlink(this, buf, size));
        });

        return promise.GetFuture();
    }

    void Close(ui64 fd) {
        Worker->SafeAddFunc([this, fd]() {
            if (IsStopped()) {
                return;
            }

            Inodes[fd].Close(this);
        });
    }

private:
    struct TInnBlob {
        TStringBuf Hash;
        TVector<ui64> ContentIds;
        ui64 NWrites = 0;
        ui64 NSkips = 0;
        TBlob Data;

        void WriteTo(TFile* f) {
            {
                TFileOutput out(*f);
                TMemoryInput in(Data.AsCharPtr(), Data.Size() - 1);
                NBlockCodecs::TDecodedInput decodedIn(&in);
                TransferData(&decodedIn, &out);
            }

            if (++NWrites + NSkips == ContentIds.size()) {
                Data.Drop();
            }
        }

        void Skip() {
            ++NSkips;
        }

        bool Downloaded() const {
            return NWrites || !Data.IsNull();
        }
    };

    struct TFileContent {
        TStringBuf Hash;
        TStringBuf Content;
        TVector<ui64> BlobIds;
        ui64 CurBlob = 0;
        THolder<TFile> File;
        THolder<NThreading::TPromise<void> > Promise;
        bool FsChecked = false;
        bool OnDisc = false;

        bool CheckSize(NStore::TDiscStore* discStore, i64 size) {
            if (size < 0) {
                return true;
            }

            i64 diskSize = discStore->Size(Hash);
            
            if (diskSize != size) {
                Cerr << "Size mismatch for " << HexEncode(Hash) << ": expected " << size << ", got " << diskSize << Endl;
                return false;
            }

            return true;
        }

        bool ChecFs(NStore::TDiscStore* discStore, i64 size = -1) {
            if (!FsChecked) {
                OnDisc = discStore->Has(Hash) && CheckSize(discStore, size);
                FsChecked = true;
            }

            return OnDisc;
        }

        bool WriteNextBlob(TVector<TInnBlob>* blobs, NStore::TDiscStore* discStore) {
            if (ChecFs(discStore)) {
                if (CurBlob < BlobIds.size()) {
                    blobs->at(BlobIds[CurBlob++]).Skip();
                    return true;
                }

                return false;
            }

            if (CurBlob == 0) {
                File = MakeHolder<TFile>(discStore->TmpPath(), CreateNew | WrOnly | Seq);
            }

            if (CurBlob == BlobIds.size()) {
                return false;
            }

            TInnBlob& blob = blobs->at(BlobIds[CurBlob]);

            if (blob.Data.IsNull()) {
                return false;
            }

            blob.WriteTo(File.Get());

            if (++CurBlob == BlobIds.size()) {
                File->Close();
                
                // TODO: remove when bug is located
                if (File->GetLength() == 0) {
                    Cerr << "Trying to put empty file to store" << Endl;
                }
                
                Y_ENSURE(discStore->PutTmpPath(Hash, File->GetName()).IsOk());
                File.Reset();
                if (Requested()) {
                    Promise->SetValue();
                    Promise.Destroy();
                }
                return false;
            }

            return true;
        }

        bool Restored() const {
            return CurBlob == BlobIds.size();
        }

        bool Requested() const {
            return !!Promise;
        }

        size_t RpcId() const {
            ui64 h;
            memcpy(&h, Hash.data(), 8);
            return h % N;
        }
    };

    struct TReadContext {
        TStringBuf Content;
        TAtomicSharedPtr<TFile> File;
    };

    struct TWriteContext {
        TAtomicSharedPtr<TFile> File;
    };

    struct TInode {
        bool Readonly = true;
        TAtomicSharedPtr<TFile> File;

        // For readonly inodes
        ui64 ContentId = 0;

        // For writable inodes
        TString Path;
        ui64 OpensStackSize = 0;

        TFileStat Stat;

        void UpdateStat() {
            TFileStat st(Path);
            Stat.ATime = st.ATime;
            Stat.MTime = st.MTime;
            Stat.Size = st.Size;
        }

        void Utimens(const struct timespec tv[2]) {
            if (!Readonly) {
                utimensat_impl(Path.data(), tv);  // TODO
            } else {
                Stat.ATime = tv[0].tv_sec;
                Stat.MTime = tv[1].tv_sec;
            }
        }

        TFileStat GetStat() {
            if (!Readonly) {
                // Y_ENSURE(NFs::Exists(Path));
                UpdateStat();
            }

            return Stat;
        }

        int Chmod(mode_t mode) {
            if ((Stat.Mode & (S_IFREG | S_IFLNK)) ^ (mode & ((S_IFREG | S_IFLNK)))) {
                return -EPERM;
            }
            Stat.Mode = mode;
            return 0;
        }

        void Unlink() {
            --Stat.NLinks;
        }

        void Link() {
            ++Stat.NLinks;
        }

        int Readlink(TFiles<N>* files, char *buf, size_t size) {
            if (!(Stat.Mode & S_IFLNK)) {
                return -EINVAL;
            }

            TString data;

            if (Readonly) {
                TFileContent& content = files->Contents[ContentId];

                if (content.Content) {
                    data = content.Content;
                } else {
                    Y_ENSURE(content.ChecFs(files->DiscStore) || content.Restored());
                    TFileInput in(files->DiscStore->InnerPath(content.Hash));
                    data = in.ReadAll();
                }
            } else {
                // Y_ENSURE(NFs::Exists(Path));
                TFileInput in(Path);
                data = in.ReadAll();
            }

            if (data.size() < size) {
                memcpy(buf, data.data(), data.size());
                buf[data.size()] = '\0';
            } else {
                memcpy(buf, data.data(), size - 1);
                buf[size - 1] = '\0';
            }

            return 0;
        }

        int Truncate(off_t off) {
            Y_ENSURE(!Readonly);

            if (OpensStackSize) {
                File.Drop();
            }

            int tc = ::truncate(Path.data(), off);
            if (tc < 0) {
                tc = -errno;
            }

            if (OpensStackSize) {
                File.Reset(new TFile(Path, OpenAlways | RdWr));
            }

            return tc;
        }

        void Open(TFiles<N>* files) {
            if (Readonly) {
                TFileContent& content = files->Contents[ContentId];

                if (content.Content) {
                    return;
                } else if (OpensStackSize++ == 0) {
                    File = new TFile(files->DiscStore->InnerPath(content.Hash), OpenExisting | RdOnly);
                }

            } else if (OpensStackSize++ == 0) {
                File = new TFile(Path, OpenAlways | RdWr);
            }
        }

        void Close(TFiles<N>* files) {
            if (Readonly) {
                TFileContent& content = files->Contents[ContentId];

                if (content.Content) {
                    return;
                } else if (--OpensStackSize == 0) {
                    File.Reset();
                }
            } else if (--OpensStackSize == 0) {
                File.Reset();
            }
        }

        TReadContext GetReadContext(TFiles<N>* files) {
            TReadContext ctx;

            if (Readonly) {
                TFileContent& content = files->Contents[ContentId];

                if (content.Content) {
                    ctx.Content = content.Content;
                } else {
                    Y_ENSURE(File);
                    ctx.File = File;
                }
            } else {
                Y_ENSURE(File);
                ctx.File = File;
            }


            return ctx;
        }

        TWriteContext GetWriteContext() {
            Y_ENSURE(!Readonly);

            TWriteContext ctx;

            Y_ENSURE(File);

            ctx.File = File;

            return ctx;
        }

        void MakeWritable(TFiles<N>* files) {
            if (!Readonly) {
                return;
            } else {
                TFileContent& content = files->Contents[ContentId];

                Path = files->DiscStore->TmpPath();
                {
                    TFileOutput out(Path);

                    if (content.Content) {
                        TMemoryInput in(content.Content.data(), content.Content.size());
                        TransferData(&in, &out);
                    } else {
                        Y_ENSURE(content.ChecFs(files->DiscStore) || content.Restored());
                        TFileInput in(files->DiscStore->InnerPath(content.Hash));
                        TransferData(&in, &out);
                    }
                }

                if (File) {
                    File.Reset(new TFile(Path, OpenAlways | RdWr));
                }

                Readonly = false;
            }
        }
    };

    bool IsStopped() {
        return static_cast<bool>(AtomicGet(Stopped));
    }

    std::pair<bool, ui64> InsertContent(const TStringBuf& hash) {
        const ui64 newContentId = Contents.size();

        auto ins = ContentIdx.insert(std::make_pair(hash, newContentId));

        if (!ins.second) {
            return std::make_pair(false, ins.first->second);
        }

        Contents.emplace_back();
        Contents.back().Hash = hash;

        return std::make_pair(true, newContentId);
    }

    ui64 InsertContentReady(const TStringBuf& hash, const TStringBuf& content) {
        bool inserted;
        ui64 id;
        std::tie(inserted, id) = InsertContent(hash);

        if (!inserted) {
            return id;
        }

        Contents[id].Content = content;
        return id;
    }

    ui64 InsertContentBlobs(const TStringBuf& hash, const TStringBuf& blobs) {
        bool inserted;
        ui64 id;
        std::tie(inserted, id) = InsertContent(hash);

        if (!inserted) {
            return id;
        }

        for (ui64 i = 0; i < blobs.size(); i += 20) {
            TStringBuf blobHash(blobs.data() + i, 20);
            ui64 blobId = Blobs.size();

            auto bins = BlobIdx.insert(std::make_pair(blobHash, blobId));

            if (!bins.second) {
                blobId = bins.first->second;
            } else {
                Blobs.emplace_back();
                Blobs.back().Hash = blobHash;
                Blobs.back().ContentIds.push_back(id);
            }

            Contents.back().BlobIds.push_back(blobId);
        }

        return id;
    }

    NThreading::TFuture<TReadContext> ReadCtx(const ui64 inode) {
        NThreading::TPromise<TReadContext> promise = NThreading::NewPromise<TReadContext>();

        Worker->SafeAddFunc([this, inode, promise]() mutable {
            if (IsStopped()) {
                promise.SetException("Stopped");
                return;
            }

            Y_ENSURE(Frozen);

            promise.SetValue(Inodes[inode].GetReadContext(this));
        });

        return promise.GetFuture();
    }

    NThreading::TFuture<TWriteContext> WriteCtx(const ui64 inode) {
        NThreading::TPromise<TWriteContext> promise = NThreading::NewPromise<TWriteContext>();

        Worker->SafeAddFunc([this, inode, promise]() mutable {
            if (IsStopped()) {
                promise.SetException("Stopped");
                return;
            }

            Y_ENSURE(Frozen);

            if (Inodes[inode].Readonly) {
                Inodes[inode].MakeWritable(this);
            }

            promise.SetValue(Inodes[inode].GetWriteContext());
        });

        return promise.GetFuture();
    }

    TAtomic Stopped = false;
    bool Frozen = false;

    TVector<TFileContent> Contents;
    TVector<TInnBlob> Blobs;
    TVector<TInode> Inodes;
    TAtomic InodesCount = 1;

    THashMap<TStringBuf, ui64, TFastHash<TStringBuf> > ContentIdx;
    THashMap<TStringBuf, ui64, TFastHash<TStringBuf> > BlobIdx;

    THolder<TObjectsRpcs<TFiles, N>> Objects3;
    NStore::TDiscStore* DiscStore;
    THolder<IThreadPool> Worker;
};

}

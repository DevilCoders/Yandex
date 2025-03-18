#include "client.h"
#include "progress.h"

#include <aapi/lib/common/async_reader.h>
#include <aapi/lib/common/async_writer.h>
#include <aapi/lib/common/git_hash.h>
#include <aapi/lib/common/walk.h>
#include <aapi/lib/node/svn_commit.fbs.h>
#include <aapi/lib/store/disc_store.h>
#include <aapi/lib/yt/async_lookups.h>

#include <contrib/libs/grpc/include/grpc++/create_channel.h>

#include <library/cpp/blockcodecs/stream.h>
#include <library/cpp/threading/future/future.h>

#include <util/stream/file.h>
#include <util/string/hex.h>
#include <util/string/split.h>
#include <util/system/fs.h>
#include <aapi/lib/node/hg_changeset.fbs.h>

namespace {

using namespace NAapi;

class TEntriesRestore {
public:
    explicit TEntriesRestore(TProgressPtr progress)
        : Progress(progress)
    {
    }

    void AddEntry(const TString& dirname, const TEntry& entry, TVector<TString>* addedBlobs = nullptr) {
        const ui64 fileId = Files.size();
        TInnFile file {JoinFsPaths(dirname, entry.Name), entry.Mode, {}, 0, Blobs, 0, {}};

        for (const TString& hash: entry.Blobs) {
            ui64 blobId;

            auto it = BlobIds.find(hash);

            if (it == BlobIds.end()) {
                blobId = Blobs.size();
                Blobs.push_back({hash, {}, 0, {}});
                BlobIds[hash] = blobId;
                if (addedBlobs) {
                    addedBlobs->push_back(hash);
                }
            } else {
                blobId = it->second;
            }

            Blobs[blobId].Ref(fileId);
            file.Ref(blobId);
        }

        Files.push_back(file);
    }

    void SetBlobData(const TString& hash, const TBlob& data) {
        auto it = BlobIds.find(hash);
        Y_ENSURE(it != BlobIds.end());
        ui64 blobId = it->second;

        Blobs[blobId].Data = data;

        for (ui64 fileId: Blobs[blobId].FileIds) {
            if (Files[fileId].Unref() == 0) {
                Progress->IncCur();
                Progress->Display(Files[fileId].Path);
            }
        }
    }

private:
    struct TInnBlob {
        TString Hash;
        TVector<ui64> FileIds;
        ui64 Refs;
        TBlob Data;

        void Ref(ui64 from) {
            FileIds.push_back(from);
            ++Refs;
        }

        ui64 Unref() {
            Y_ENSURE(Refs > 0);

            if (--Refs == 0) {
                Data.Drop();
            }

            return Refs;
        }
    };

    struct TInnFile {
        TString Path;
        EEntryMode Mode;
        TVector<ui64> BlobIds;
        ui64 Refs;
        TVector<TInnBlob>& BlobsStore;
        ui64 Pos;
        TFile File;

        void Ref(ui64 from) {
            BlobIds.push_back(from);
            ++Refs;
        }

        ui64 Unref() {
            Y_ENSURE(Refs > 0);

            while (WriteNextBlob() && Refs > 0) {
            }

            return Refs;
        }

        bool WriteNextBlob() {
            if (Pos == BlobIds.size()) {
                return false;
            }

            if (Mode == EEntryMode::EEM_LINK) {
                Y_ENSURE(BlobIds.size() == 1);
                const ui64 blobId = BlobIds.front();
                TInnBlob& blob = BlobsStore[blobId];
                Y_ENSURE(!blob.Data.IsNull());
                TMemoryInput in(blob.Data.AsCharPtr(), blob.Data.Size() - 1);
                NBlockCodecs::TDecodedInput decodedIn(&in);
                TStringStream symlinkDest;
                TransferData(&decodedIn, &symlinkDest);
                NFs::SymLink(symlinkDest.ReadAll(), Path);
                --Refs;
                ++Pos;
                blob.Unref();
                return true;
            }

            const ui64 blobId = BlobIds[Pos];
            TInnBlob& blob = BlobsStore[blobId];

            if (blob.Data.IsNull()) {
                return false;
            }

            if (Pos == 0) {
                File = TFile(Path, CreateAlways | WrOnly | Seq);
            }

            {
                TFileOutput out(File);
                TMemoryInput in(blob.Data.AsCharPtr(), blob.Data.Size() - 1);
                NBlockCodecs::TDecodedInput decodedInput(&in);
                TransferData(&decodedInput, &out);
                --Refs;
                ++Pos;
                blob.Unref();
            }

            if (Pos == BlobIds.size()) {
                File.Close();
                if (Mode == EEntryMode::EEM_EXEC) {
                    TFileStat stat(Path);
                    Chmod(Path.data(), stat.Mode | S_IEXEC);
                }
            }

            return true;
        }
    };

    TProgressPtr Progress;
    TVector<TInnFile> Files;
    TVector<TInnBlob> Blobs;
    THashMap<TString, ui64> BlobIds;
};

}  // namespace

namespace NAapi {

TVcsClient::TVcsClient(const TString& proxyAddr) {
    grpc::ChannelArguments channelArgs;
    channelArgs.SetMaxReceiveMessageSize(35 * 1024 * 1024);
    std::shared_ptr<grpc::Channel> channel = grpc::CreateCustomChannel(proxyAddr.data(), grpc::InsecureChannelCredentials(), channelArgs);
    Stub = THolder<NVcs::Vcs::Stub>(NVcs::Vcs::NewStub(channel).release());
}

TSvnCommitInfo TVcsClient::GetSvnCommit(ui64 revision) {
    grpc::ClientContext ctx;
    THolder<TObjects2Stream> stream(Stub->Objects2(&ctx).release());
    return GetSvnCommit(revision, stream.Get());
}

THgChangesetInfo TVcsClient::GetHgChangeset(const TString& changeset) {
    grpc::ClientContext ctx;
    THolder<TObjects2Stream> stream(Stub->Objects2(&ctx).release());
    return GetHgChangeset(changeset, stream.Get());
}

TMaybe<TString> TVcsClient::GetObject(const TString& hash, TObjects2Stream* stream) {
    THash h;
    h.SetHash(hash);
    stream->Write(h);

    TObject obj;
    if (!stream->Read(&obj)) {
        grpc::Status st = stream->Finish();
        Y_ENSURE(!st.ok());
        if (st.error_code() == grpc::StatusCode::NOT_FOUND) {
            return TMaybe<TString>();
        } else {
            ythrow TGrpcError(st.error_code(), TString(st.error_message()));
        }
    }

    return obj.GetBlob();
}

TString TVcsClient::EnsureGetObject(const TString& hash, TObjects2Stream* stream) {
    TMaybe<TString> obj = GetObject(hash, stream);
    Y_ENSURE(!obj.Empty());
    return *obj;
}

TSvnCommitInfo TVcsClient::GetSvnCommit(ui64 revision, TObjects2Stream* stream) {
    TString revisionHash = GitLikeHash(ToString(revision));
    TMaybe<TString> revisionDump = GetObject(revisionHash, stream);

    if (revisionDump.Empty()) {
        ythrow TNoSuchRevisionError(revision);
    }

    TString fbsDump;
    {
        TMemoryInput input(revisionDump->data(), revisionDump->size() - 1);
        NBlockCodecs::TDecodedInput decodedInput(&input);
        TStringOutput output(fbsDump);
        TransferData(&decodedInput, &output);
    }

    auto fbsCi = NNode::GetSvnCommit(fbsDump.data());
    TSvnCommitInfo ci;
    ci.Revision = fbsCi->revision();
    ci.Author = TString(fbsCi->author()->c_str());
    ci.Date = TString(fbsCi->date()->c_str());
    ci.Message = TString(fbsCi->msg()->c_str());
    ci.TreeHash = TString(reinterpret_cast<const char*>(fbsCi->tree()->data()), fbsCi->tree()->size());
    ci.ParentHash = TString(reinterpret_cast<const char*>(fbsCi->parent()->data()), fbsCi->parent()->size());

    return ci;
}

THgChangesetInfo TVcsClient::GetHgChangeset(const TString& changeset, TObjects2Stream* stream) {
    Y_ENSURE(changeset.size() == 20);
    TMaybe<TString> csDump = GetObject(changeset, stream);

    if (csDump.Empty()) {
        ythrow TNoSuchChangesetError(HexEncode(changeset));
    }

    TString fbsDump;
    {
        TMemoryInput input(csDump->data(), csDump->size() - 1);
        NBlockCodecs::TDecodedInput decodedInput(&input);
        TStringOutput output(fbsDump);
        TransferData(&decodedInput, &output);
    }

    auto fbsCs = NNode::GetHgChangeset(fbsDump.data());
    THgChangesetInfo cs;
    cs.Hash = TString(reinterpret_cast<const char*>(fbsCs->hash()->data()), fbsCs->hash()->size());
    cs.Author = TString(fbsCs->author()->c_str());
    cs.Date = TString(fbsCs->date()->c_str());
    cs.Message = TString(fbsCs->msg()->c_str());
    cs.Branch = TString(fbsCs->branch()->c_str());
    cs.Extra = TString(fbsCs->extra()->c_str());
    cs.TreeHash = TString(reinterpret_cast<const char*>(fbsCs->tree()->data()), fbsCs->tree()->size());
    cs.Parents = TVector<TString>();
    for (size_t i = 0; i < fbsCs->parents()->size(); i += 20) {
        cs.Parents.emplace_back(reinterpret_cast<const char*>(fbsCs->parents()->data()) + i, 20);
    }
    cs.ClosesBranch = static_cast<bool>(fbsCs->close_branch());

    return cs;
}

TPathInfo TVcsClient::GetSvnPath(const TString& path, ui64 revision) {
    grpc::ClientContext ctx;
    THolder<TObjects2Stream> stream(Stub->Objects2(&ctx).release());

    TMaybe<TSvnCommitInfo> svnCi = GetSvnCommit(revision, stream.Get());

    if (svnCi.Empty()) {
        ythrow TNoSuchRevisionError(revision);
    }
    if (AsciiHasPrefix(ToString(TFsPath(path).Fix()), "/secure")) {
        ythrow TNoSuchPathError(svnCi->TreeHash, path);
    }

    if (const auto& pathInfo = GetPath(path, svnCi->TreeHash, stream.Get())) {
        return *pathInfo;
    }

    ythrow TNoSuchPathError(svnCi->TreeHash, path);
}

TPathInfo TVcsClient::GetHgPath(const TString& path, const TString& changeset) {
    grpc::ClientContext ctx;
    THolder<TObjects2Stream> stream(Stub->Objects2(&ctx).release());

    TMaybe<THgChangesetInfo> cs = GetHgChangeset(changeset, stream.Get());

    if (cs.Empty()) {
        ythrow TNoSuchChangesetError(HexEncode(changeset));
    }

    if (const auto& pathInfo = GetPath(path, cs->TreeHash, stream.Get())) {
        return *pathInfo;
    }

    ythrow TNoSuchPathError(cs->TreeHash, path);
}

TMaybe<TPathInfo> TVcsClient::GetPath(const TString& path, const TString& rootHash, TObjects2Stream* stream) {
    TString curdir = EnsureGetObject(rootHash, stream);
    TVector<TStringBuf> parts;
    Split(path, "/", parts);

    for (size_t i = 0; i + 1 < parts.size(); ++i) {
        const TStringBuf& part = parts[i];

        TEntriesIter iter(curdir.data(), TEntriesIter::EIterMode::EIM_DIRS);
        TEntry entry;
        bool found = false;
        while (iter.Next(entry)) {
            if (entry.Name == part) {
                found = true;
                curdir = EnsureGetObject(entry.Hash, stream);
                break;
            }
        }

        if (!found) {
            return TMaybe<TPathInfo>();
        }
    }

    if (parts.size() == 0) {
        TPathInfo info;
        info.Path = path;
        info.Mode = EEntryMode::EEM_DIR;
        info.Hash = rootHash;
        return info;
    }

    TEntriesIter iter(curdir.data());
    TEntry entry;
    while (iter.Next(entry)) {
        if (entry.Name == parts.back()) {
            TPathInfo info;
            info.Path = path;
            info.Mode = entry.Mode;
            info.Hash = entry.Hash;
            info.Blobs = entry.Blobs;
            return info;
        }
    }

    return TMaybe<TPathInfo>();
}

TVector<TPathInfo> TVcsClient::ListSvnPath(const TString& path, ui64 revision, bool recursive) {
    return ListPath(GetSvnPath(path, revision), recursive);
}

TVector<TPathInfo> TVcsClient::ListHgPath(const TString& path, const TString& changeset, bool recursive) {
    return ListPath(GetHgPath(path, changeset), recursive);
}

TVector<TPathInfo> TVcsClient::ListPath(const TPathInfo& root, bool recursive) {
    TVector<TPathInfo> result;
    TEntry entry;

    if (root.Mode != EEntryMode::EEM_DIR) {
        result.push_back(root);
        return result;
    }

    grpc::ClientContext ctx;
    THash request;
    request.SetHash(root.Hash);

    if (!recursive) {
        result.push_back(root);
        result.back().Path = ".";
        THolder<TObjects2Stream> stream(Stub->Objects2(&ctx).release());
        stream->Write(request);
        stream->WritesDone();
        TObject obj;
        stream->Read(&obj);
        TEntriesIter iter(obj.GetBlob().data());
        while (iter.Next(entry)) {
            TPathInfo fileInfo;
            fileInfo.Path = JoinFsPaths(".", entry.Name);
            fileInfo.Mode = entry.Mode;
            fileInfo.Hash = entry.Hash;
            fileInfo.Blobs = entry.Blobs;
            result.push_back(fileInfo);
        }
        return result;
    }

    THolder<grpc::ClientReader<TDirectories> > reader(Stub->Walk(&ctx, request).release());

    TDirectories dirs;
    while (reader->Read(&dirs)) {
        for (const TDirectory& dir: dirs.GetDirectories()) {
            TPathInfo dirInfo;
            dirInfo.Path = dir.GetPath();
            dirInfo.Hash = dir.GetHash();
            dirInfo.Mode = EEntryMode::EEM_DIR;
            result.push_back(dirInfo);

            TEntriesIter iter(dir.GetBlob().data(), TEntriesIter::EIterMode::EIM_FILES);
            while (iter.Next(entry)) {
                TPathInfo fileInfo;
                fileInfo.Path = JoinFsPaths(dir.GetPath(), entry.Name);
                fileInfo.Mode = entry.Mode;
                fileInfo.Hash = entry.Hash;
                fileInfo.Blobs = entry.Blobs;
                result.push_back(fileInfo);
            }
        }
    }

    grpc::Status st = reader->Finish();

    if (!st.ok()) {
        ythrow TGrpcError(st.error_code(), TString(st.error_message()));
    }

    return result;
}

void TVcsClient::ExportDir(const TPathInfo& entry, const TString& destDir, const TString& storePath) {
    const TString& rootKey = entry.Hash;

    using TWalkReader = grpc::ClientReader<TDirectories>;
    using TObjects3Stream = grpc::ClientReaderWriter<THashes, TObjects>;

    grpc::ClientContext walkContext;
    THash walkRequest;
    walkRequest.SetHash(rootKey);
    THolder<TWalkReader> walkReader(Stub->Walk(&walkContext, walkRequest).release());
    TAsyncReader<TWalkReader, TDirectories> asyncWalkReader(walkReader.Get(), 0);

    grpc::ClientContext objectsContext;
    THolder<TObjects3Stream> objectsStream(Stub->Objects3(&objectsContext).release());
    TAsyncWriter<TObjects3Stream, THashes, TStopGrpcClientWriter<TObjects3Stream> > asyncObjectsWriter(objectsStream.Get(), 0);
    TAsyncReader<TObjects3Stream, TObjects> asyncObjectsReader(objectsStream.Get(), 128);

    TProgressPtr progress(new TProgress());
    THashes hashes;
    TEntriesRestore restore(progress);

    THolder<NStore::TDiscStore> discStore;
    if (!storePath.empty()) {
        discStore = MakeHolder<NStore::TDiscStore>(storePath);
    }

    THolder<IThreadPool> discReaders;
    NThreading::TBlockingQueue<TRow> discReads(128);
    if (discStore) {
        discReaders = MakeHolder<TThreadPool>(TThreadPool::TParams().SetBlocking(true).SetCatching(true));
        discReaders->Start(3);
    }

    TDirectories directoriesChunk;
    while (asyncWalkReader.Read(directoriesChunk)) {
        for (const TDirectory& directory: directoriesChunk.GetDirectories()) {
            progress->IncTotal();

            const TString directoryPath = JoinFsPaths(destDir, directory.GetPath());

            TEntriesIter entriesIter(directory.GetBlob().data(), TEntriesIter::EIterMode::EIM_FILES);
            TEntry file;
            while (entriesIter.Next(file)) {
                progress->IncTotal();

                TVector<TString> addedBlobs;
                restore.AddEntry(directoryPath, file, &addedBlobs);
                for (const TString& hash: addedBlobs) {
                    if (discStore && discStore->Has(hash)) {
                        discReaders->SafeAddFunc([hash, store = discStore.Get(), &discReads] () {
                            TString data;
                            if (store->Get(hash, data).IsOk()) {
                                discReads.Push(TRow(hash, data));
                            } else {
                                discReads.Push(TRow(hash, TString()));
                            }
                        });
                    } else {
                        *hashes.AddHashes() = hash;
                    }
                }
            }

            if (hashes.HashesSize() >= 128) {
                asyncObjectsWriter.Write(hashes);
                hashes.ClearHashes();
            }

            if (!entriesIter.HasDirChild()) {
                Y_ENSURE(NFs::MakeDirectoryRecursive(directoryPath));
            }

            progress->IncCur();
            progress->Display(directoryPath);
        }
    }

    if (discStore) {
        while (discReaders->Size() || !discReads.Empty()) {
            const TRow x = *discReads.Pop();

            if (!x.second.empty()) {
                restore.SetBlobData(x.first, TBlob::FromStringSingleThreaded(x.second));
            } else {
                *hashes.AddHashes() = x.first;
            }

            if (hashes.HashesSize() >= 128) {
                asyncObjectsWriter.Write(hashes);
                hashes.ClearHashes();
            }
        }

        discReaders->Stop();

        while (!discReads.Empty()) {
            const TRow x = *discReads.Pop();

            if (!x.second.empty()) {
                restore.SetBlobData(x.first, TBlob::FromStringSingleThreaded(x.second));
            } else {
                *hashes.AddHashes() = x.first;
            }

            if (hashes.HashesSize() >= 128) {
                asyncObjectsWriter.Write(hashes);
                hashes.ClearHashes();
            }
        }
    }

    if (hashes.HashesSize()) {
        asyncObjectsWriter.Write(hashes);
    }

    asyncWalkReader.Join();
    grpc::Status walkStatus = walkReader->Finish();  // analyze status
    asyncObjectsWriter.Stop();

    TObjects objects;
    while (asyncObjectsReader.Read(objects)) {
        for (size_t i = 0; i < objects.DataSize(); i += 2) {
            if (objects.GetData(i).empty()) {
                ythrow yexception() << "hash: " << HexEncode(objects.GetData(i + 1)) << ", error: " << objects.GetData(i + 2);
            }
            restore.SetBlobData(objects.GetData(i), TBlob::FromStringSingleThreaded(objects.GetData(i + 1)));
            if (discStore) {
                discStore->Put(objects.GetData(i), objects.GetData(i + 1));
            }
        }
    }

    asyncObjectsWriter.Join();
    asyncObjectsReader.Join();

    grpc::Status objectsStatus = objectsStream->Finish();  // analyze status

    if (!walkStatus.ok()) {
        ythrow TGrpcError(walkStatus.error_code(), TString(walkStatus.error_message()));
    }

    if (!objectsStatus.ok()) {
        ythrow TGrpcError(objectsStatus.error_code(), TString(objectsStatus.error_message()));
    }
}

void TVcsClient::ExportFile(const TPathInfo& entry, const TString& dest, const TString& storePath) {
    grpc::ClientContext ctx;
    THolder<TObjects2Stream> stream(Stub->Objects2(&ctx).release());

    THolder<NStore::TDiscStore> discStore;
    if (!storePath.empty()) {
        discStore = MakeHolder<NStore::TDiscStore>(storePath);
    }

    if (entry.Mode == EEntryMode::EEM_LINK) {
        Y_ENSURE(entry.Blobs.size() == 1);

        TString blobDump;

        if (discStore && discStore->Get(entry.Blobs.front(), blobDump).IsOk()) {
        } else {
            blobDump = EnsureGetObject(entry.Blobs.front(), stream.Get());
            if (discStore) {
                discStore->Put(entry.Blobs.front(), blobDump);
            }
        }

        TString symlinkDest;
        {
            TMemoryInput input(blobDump.data(), blobDump.size() - 1);
            NBlockCodecs::TDecodedInput decodedInput(&input);
            TStringOutput output(symlinkDest);
            TransferData(&decodedInput, &output);
        }
        NFs::SymLink(symlinkDest, dest);

    } else {
        {
            TFileOutput output(dest);  // TODO atomic

            for (const TString& blobHash: entry.Blobs) {
                TString blobDump;
                if (discStore && discStore->Get(blobHash, blobDump).IsOk()) {
                } else {
                    blobDump = EnsureGetObject(blobHash, stream.Get());
                    if (discStore) {
                        discStore->Put(blobHash, blobDump);
                    }
                }
                TMemoryInput input(blobDump.data(), blobDump.size() - 1);
                NBlockCodecs::TDecodedInput decodedInput(&input);
                TransferData(&decodedInput, &output);
            }
        }

        if (entry.Mode == EEntryMode::EEM_EXEC) {
            TFileStat stat(dest);
            Chmod(dest.data(), stat.Mode | S_IEXEC);
        }
    }
}

void TVcsClient::Export(const TPathInfo& entry, const TString& dest, const TString& storePath) {
    if (NFs::Exists(dest)) {
        ythrow TPathExistsError(dest);
    }

    if (entry.Mode == EEntryMode::EEM_DIR) {
        ExportDir(entry, dest, storePath);
    } else {
        ExportFile(entry, dest, storePath);
    }
}

void TVcsClient::Export(const TString& path, ui64 revision, const TString& dest, const TString& storePath) {
    Export(GetSvnPath(path, revision), dest, storePath);
}

void TVcsClient::Export(const TString& path, const TString& changeset, const TString& dest, const TString& storePath) {
    Export(GetHgPath(path, changeset), dest, storePath);
}

ui64 TVcsClient::GetSvnHead() {
    grpc::ClientContext ctx;
    TEmpty request;
    TRevision response;
    grpc::Status st = Stub->SvnHead(&ctx, request, &response);

    if (!st.ok()) {
        ythrow TGrpcError(st.error_code(), TString(st.error_message()));
    }

    return response.GetRevision();
}

TString TVcsClient::GetHgId(const TString& name) {
    grpc::ClientContext ctx;
    TName request;
    request.SetName(name);
    THash response;
    grpc::Status st = Stub->HgId(&ctx, request, &response);

    if (!st.ok()) {
        ythrow TGrpcError(st.error_code(), TString(st.error_message()));
    }

    return response.GetHash();
}

}  // namespace NAapi

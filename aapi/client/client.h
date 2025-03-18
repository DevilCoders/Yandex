#pragma once

#include <aapi/lib/proto/vcs.grpc.pb.h>
#include <aapi/lib/common/object_types.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/maybe.h>

namespace NAapi {

class TGrpcError : public yexception {
public:
    TGrpcError(grpc::StatusCode code, const TString message)
        : Code(code)
        , Message(message)
    {
    }

    grpc::StatusCode Code;
    TString Message;
};

class TNoSuchRevisionError : public yexception {
public:
    explicit TNoSuchRevisionError(ui64 revision)
        : Revision(revision)
    {
    }

    ui64 Revision;
};

class TNoSuchChangesetError : public yexception {
public:
    explicit TNoSuchChangesetError(const TString& changeset)
        : Changeset(changeset)
    {
    }

    TString Changeset;
};

class TNoSuchPathError : public yexception {
public:
    TNoSuchPathError(const TString& rootHash, const TString& path)
        : RootHash(rootHash)
        , Path(path)
    {
    }

    TString RootHash;
    TString Path;
};

class TPathExistsError : public yexception {
public:
    explicit TPathExistsError(const TString& path)
        : Path(path)
    {
    }

    TString Path;
};

class TVcsClient {
public:
    explicit TVcsClient(const TString& proxyAddr);

    TSvnCommitInfo GetSvnCommit(ui64 revision);
    THgChangesetInfo GetHgChangeset(const TString& changeset);

    TPathInfo GetSvnPath(const TString& path, ui64 revision);
    TPathInfo GetHgPath(const TString& path, const TString& changeset);

    TVector<TPathInfo> ListSvnPath(const TString& path, ui64 revision, bool recursive = false);
    TVector<TPathInfo> ListHgPath(const TString& path, const TString& changeset, bool recursive = false);

    void Export(const TPathInfo& entry, const TString& dest, const TString& storePath = TString());
    void Export(const TString& path, ui64 revision, const TString& dest, const TString& storePath = TString());
    void Export(const TString& path, const TString& changeset, const TString& dest, const TString& storePath = TString());

    ui64 GetSvnHead();
    TString GetHgId(const TString& name);

private:
    using TObjects2Stream = grpc::ClientReaderWriter<THash, TObject>;

    TVector<TPathInfo> ListPath(const TPathInfo& entry, bool recursive);
    TSvnCommitInfo GetSvnCommit(ui64 revision, TObjects2Stream* stream);
    THgChangesetInfo GetHgChangeset(const TString& changeset, TObjects2Stream* stream);
    TMaybe<TString> GetObject(const TString& hash, TObjects2Stream* stream);
    TString EnsureGetObject(const TString& hash, TObjects2Stream* stream);
    TMaybe<TPathInfo> GetPath(const TString& path, const TString& rootHash, TObjects2Stream* stream);

    void ExportDir(const TPathInfo& entry, const TString& dest, const TString& storePath = TString());
    void ExportFile(const TPathInfo& entry, const TString& dest, const TString& storePath = TString());

private:
    THolder<NVcs::Vcs::Stub> Stub;
};

}  // namespace NAapi

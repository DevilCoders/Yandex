#include <aapi/client/client.h>
#include <aapi/fuse_client/interface.h>
#include <aapi/lib/hg/hg.h>

#include <util/string/hex.h>
#include <util/string/cast.h>
#include <util/folder/path.h>

#include <library/cpp/getopt/last_getopt.h>

#include <mapreduce/yt/interface/client.h>

using namespace NLastGetopt;

TString ToLower(const TString& s) {
    TString ret(s);
    ret.to_lower();
    return ret;
}

TString ToUpper(const TString& s) {
    TString ret(s);
    ret.to_upper();
    return ret;
}

int PrintSvnCommit(NAapi::TVcsClient* client, ui64 revision) {
    TMaybe<NAapi::TSvnCommitInfo> ci = client->GetSvnCommit(revision);
    if (ci.Empty()) {
        Cerr << "No such revision " << revision << Endl;
        return 1;
    }

    Cout << "Revision: " << ci->Revision << '\n'
         << "Author: " << ci->Author << '\n'
         << "Message: " << ci->Message << '\n'
         << "Date: " << ci->Date << '\n'
         << "Tree: " << ToLower(HexEncode(ci->TreeHash)) << '\n'
         << "Parent: " << ToLower(HexEncode(ci->ParentHash)) << Endl;

    return 0;
}

int PrintHgChageset(NAapi::TVcsClient* client, const TString& changeset) {
    Y_ENSURE(changeset.size() == 20);
    TMaybe<NAapi::THgChangesetInfo> cs = client->GetHgChangeset(changeset);

    if (cs.Empty()) {
        Cerr << "No such changeset " << HexEncode(changeset) << Endl;
        return 1;
    }

    Cout << "Hash: " << ToLower(HexEncode(cs->Hash)) << '\n'
         << "Author: " << cs->Author << '\n'
         << "Message: " << cs->Message << '\n'
         << "Date: " << cs->Date << '\n'
         << "Branch: " << cs->Branch << '\n'
         << "Extra: " << cs->Extra << '\n'
         << "Tree: " << ToLower(HexEncode(cs->TreeHash)) << '\n'
         << "Closes branch: " << static_cast<int>(cs->ClosesBranch) << '\n';

    for (size_t i = 0; i < cs->Parents.size(); ++i) {
        Cout << "p" << static_cast<int>(i + 1) << ": " << ToLower(HexEncode(cs->Parents[i])) << '\n';
    }

    return 0;
}

void PrintEntry(const NAapi::TPathInfo& meta) {
    TString type;
    if (meta.Mode == NAapi::EEntryMode::EEM_DIR) {
        type = "directory";
    } else if (meta.Mode == NAapi::EEntryMode::EEM_EXEC) {
        type = "executable";
    } else if (meta.Mode == NAapi::EEntryMode::EEM_LINK) {
        type = "symlink";
    } else if (meta.Mode == NAapi::EEntryMode::EEM_REG){
        type = "file";
    } else {
        ythrow yexception();
    }

    Cout << "Path: " << meta.Path << '\n'
         << "Type: " << type << '\n'
         << "Hash: " << ToLower(HexEncode(meta.Hash)) << '\n'
         << "Blobs: ";

    for (const TString& blob: meta.Blobs) {
        Cout << ToLower(HexEncode(blob)) << " ";
    }

    Cout << Endl;
}

int PrintPathInfo(NAapi::TVcsClient* client, const TString& path, ui64 revision) {
    NAapi::TPathInfo meta = client->GetSvnPath(path, revision);
    PrintEntry(meta);
    Cout << Endl;
    return 0;
}

int PrintPathInfo(NAapi::TVcsClient* client, const TString& path, const TString& changeset) {
    NAapi::TPathInfo meta = client->GetHgPath(path, changeset);
    PrintEntry(meta);
    Cout << Endl;
    return 0;
}

int ListSvnPath(NAapi::TVcsClient* client, const TString& path, ui64 revision, bool recursive) {
    TVector<NAapi::TPathInfo> metas = client->ListSvnPath(path, revision, recursive);

    for (const NAapi::TPathInfo& meta: metas) {
        PrintEntry(meta);
        Cout << Endl;
    }

    return 0;
}

int ListHgPath(NAapi::TVcsClient* client, const TString& path, const TString& changeset, bool recursive) {
    TVector<NAapi::TPathInfo> metas = client->ListHgPath(path, changeset, recursive);

    for (const NAapi::TPathInfo& meta: metas) {
        PrintEntry(meta);
        Cout << Endl;
    }

    return 0;
}

void RunFuse(const TString& subcommand, const TString& dest, THolder<NAapi::TFileSystemHolder>& fileSystem) {
    struct fuse_args fuseArgs = CreateFuseArgs({subcommand, "-f", "-ouse_ino", "-odefault_permissions", dest});
    struct fuse_operations operations = GetAapiFuseOperations();
    fuse_main(fuseArgs.argc, fuseArgs.argv, &operations, fileSystem.Get());
}

TOpts InitCommonOpts(TString& server) {
    TOpts opts;
    opts.AddHelpOption('h');
    opts.AddLongOption('p', "proxy", "vcs proxy")
        .Optional()
        .StoreResult(&server)
        .DefaultValue("aapi.vcs.yandex.net:7777");
    return opts;
}

int SvnHead(int argc, char** argv) {
    TString server;
    TOpts opts = InitCommonOpts(server);
    TOptsParseResult res(&opts, argc, argv);
    Cout << NAapi::TVcsClient(server).GetSvnHead() << Endl;
    return 0;
}

int SvnCommitInfo(int argc, char** argv) {
    ui64 revision;
    TString server;
    TOpts opts = InitCommonOpts(server);
    opts.AddLongOption('r', "revision", "svn revision")
        .Required()
        .StoreResult(&revision);
    TOptsParseResult res(&opts, argc, argv);

    NAapi::TVcsClient client(server);
    return PrintSvnCommit(&client, revision);
}

int SvnPathInfo(int argc, char** argv) {
    ui64 revision = 0;
    int rc = 0;

    TString server;
    TOpts opts = InitCommonOpts(server);
    opts.AddLongOption('r', "revision", "svn revision")
        .Optional()
        .StoreResult(&revision);
    TOptsParseResult res(&opts, argc, argv);

    NAapi::TVcsClient client(server);
    if (!revision) {
        revision = client.GetSvnHead();
    }
    for (const TString& path: res.GetFreeArgs()) {
        rc += PrintPathInfo(&client, path, revision);
    }

    Cerr << "Revision: " << revision << Endl;
    return rc != 0;
}

int SvnList(int argc, char** argv) {
    ui64 revision = 0;
    bool recursive = false;
    int rc = 0;

    TString server;
    TOpts opts = InitCommonOpts(server);
    opts.AddLongOption('r', "revision", "svn revision")
        .Optional()
        .StoreResult(&revision);
    opts.AddLongOption("recursive", "list all subdirectories recursively")
        .Optional()
        .NoArgument()
        .SetFlag(&recursive);
    TOptsParseResult res(&opts, argc, argv);

    NAapi::TVcsClient client(server);
    if (!revision) {
        revision = client.GetSvnHead();
    }
    for (const TString& path: res.GetFreeArgs()) {
        rc += ListSvnPath(&client, path, revision, recursive);
    }

    Cerr << "Revision: " << revision << Endl;
    return rc != 0;
}

int SvnExport(int argc, char** argv) {
    TString storePath;
    ui64 revision = 0;

    TString server;
    TOpts opts = InitCommonOpts(server);
    opts.AddLongOption('r', "revision", "svn revision")
        .Optional()
        .StoreResult(&revision);
    opts.AddLongOption('s', "store", "storage path")
        .Optional()
        .StoreResult(&storePath);
    TOptsParseResult res(&opts, argc, argv);

    if (res.GetFreeArgCount() < 1 || res.GetFreeArgCount() > 2) {
        Cerr << "Usage: vcs svn-export [src] [optional:dest]" << Endl;
        return 1;
    }

    NAapi::TVcsClient client(server);
    if (!revision) {
        revision = client.GetSvnHead();
    }
    TVector<TString> args = res.GetFreeArgs();
    const TString& src = args.front();
    const TString dest = args.size() == 2 ? args.back() : TFsPath(src).Basename();

    client.Export(src, revision, dest, storePath);
    Cerr << "Revision: " << revision << Endl;
    return 0;
}

int SvnFuseMount(int argc, char** argv) {
    TString storePath;
    ui64 revision = 0;

    TString server;
    TOpts opts = InitCommonOpts(server);
    opts.AddLongOption('r', "revision", "svn revision")
        .Optional()
        .StoreResult(&revision);
    opts.AddLongOption('s', "store", "storage path")
        .Required()
        .StoreResult(&storePath);
    TOptsParseResult res(&opts, argc, argv);

    if (res.GetFreeArgCount() < 1 || res.GetFreeArgCount() > 2 || !res.Has('s')) {
        Cerr << "Usage: vcs svn-fuse-mount [src] [optional:dest] -s [storePath]" << Endl;
        return 1;
    }

    THolder<NAapi::TVcsClient> client = MakeHolder<NAapi::TVcsClient>(server);
    if (!revision) {
        revision = client->GetSvnHead();
    }

    TVector<TString> args = res.GetFreeArgs();
    const TString& src = args.front();
    const TString dest = args.size() == 2 ? args.back() : TFsPath(src).Basename();

    auto pathInfo = client->GetSvnPath(src, revision);
    auto commitInfo = client->GetSvnCommit(revision);

    Y_ENSURE(pathInfo.Mode == NAapi::EEntryMode::EEM_DIR);

    client.Destroy();  // This really closes tcp connection with grpc-server

    THolder<NAapi::TFileSystemHolder> fileSystem = InitSvn(pathInfo, commitInfo, storePath, server);
    Cerr << "Mount " << src << "@" << revision << " to " << dest << Endl;

    RunFuse(argv[0], dest, fileSystem);
    return 0;
}

int HgHeads(int, char**) {
    Cerr << "Hg heads is not imlemented yet!" << Endl;
    return 2;
}

int HgChangesetInfo(int argc, char** argv) {
    TString c;

    TString server;
    TOpts opts = InitCommonOpts(server);
    opts.AddLongOption('c', "changeset", "hg changeset")
        .Required()
        .StoreResult(&c);
    TOptsParseResult res(&opts, argc, argv);

    if (c.size() != 40) {
        Cerr << "Expected 20-bytes hex string, got: " << c << Endl;
    }

    NAapi::TVcsClient client(server);
    return PrintHgChageset(&client, HexDecode(c));
}

int HgList(int argc, char** argv) {
    TString cs;
    bool recursive = false;
    int rc = 0;

    TString server;
    TOpts opts = InitCommonOpts(server);
    opts.AddLongOption('c', "changeset", "hg changeset")
        .Required()
        .StoreResult(&cs);
    opts.AddLongOption("recursive", "list all subdirectories recursively")
        .Optional()
        .NoArgument()
        .SetFlag(&recursive);
    TOptsParseResult res(&opts, argc, argv);

    NAapi::TVcsClient client(server);

    for (const TString& path: res.GetFreeArgs()) {
        rc += ListHgPath(&client, path, HexDecode(cs), recursive);
    }

    Cerr << "Changeset: " << cs << Endl;
    return rc != 0;
}

int HgPathInfo(int argc, char** argv) {
    TString cs;
    int rc = 0;

    TString server;
    TOpts opts = InitCommonOpts(server);
    opts.AddLongOption('c', "changeset", "hg changeset")
        .Required()
        .StoreResult(&cs);
    TOptsParseResult res(&opts, argc, argv);

    NAapi::TVcsClient client(server);

    for (const TString& path: res.GetFreeArgs()) {
        rc += PrintPathInfo(&client, path, HexDecode(cs));
    }

    Cerr << "Changeset: " << cs << Endl;
    return rc != 0;
}

int HgExport(int argc, char** argv) {
    TString storePath;
    TString changeset;

    TString server;
    TOpts opts = InitCommonOpts(server);
    opts.AddLongOption('c', "changeset", "hg changeset")
        .Required()
        .StoreResult(&changeset);
    opts.AddLongOption('s', "store", "storage path")
        .Optional()
        .StoreResult(&storePath);
    TOptsParseResult res(&opts, argc, argv);

    if (res.GetFreeArgCount() < 1 || res.GetFreeArgCount() > 2) {
        Cerr << "Usage: vcs hg-export [src] [optional:dest]" << Endl;
        return 1;
    }

    NAapi::TVcsClient client(server);

    TVector<TString> args = res.GetFreeArgs();
    const TString& src = args.front();
    const TString dest = args.size() == 2 ? args.back() : TFsPath(src).Basename();

    client.Export(src, HexDecode(changeset), dest, storePath);
    Cerr << "Changeset: " << changeset << Endl;
    return 0;
}

int HgId(int argc, char** argv) {
    TString server;
    TOpts opts = InitCommonOpts(server);
    TOptsParseResult res(&opts, argc, argv);

    if (res.GetFreeArgCount() != 1) {
        Cerr << "Usage: vcs hg-id [name]" << Endl;
        return 1;
    }

    const TString name = res.GetFreeArgs().front();

    NAapi::TVcsClient client(server);

    Cout << ToLower(HexEncode(client.GetHgId(name))) << Endl;

    return 0;
}

int HgFuseMount(int argc, char** argv) {
    TString storePath;
    TString cs;

    TString server;
    TOpts opts = InitCommonOpts(server);
    opts.AddLongOption('c', "changeset", "hg changeset")
        .Required()
        .StoreResult(&cs);
    opts.AddLongOption('s', "store", "storage path")
        .Required()
        .StoreResult(&storePath);
    TOptsParseResult res(&opts, argc, argv);

    if (res.GetFreeArgCount() < 1 || res.GetFreeArgCount() > 2 || !res.Has('s')) {
        Cerr << "Usage: vcs hg-fuse-mount [src] [optional:dest] -s [storePath]" << Endl;
        return 1;
    }

    THolder<NAapi::TVcsClient> client = MakeHolder<NAapi::TVcsClient>(server);

    TVector<TString> args = res.GetFreeArgs();
    const TString& src = args.front();
    const TString dest = args.size() == 2 ? args.back() : TFsPath(src).Basename();

    auto pathInfo = client->GetHgPath(src, HexDecode(cs));
    auto commitInfo = client->GetHgChangeset(HexDecode(cs));

    Y_ENSURE(pathInfo.Mode == NAapi::EEntryMode::EEM_DIR);

    client.Destroy();  // This really closes tcp connection with grpc-server

    THolder<NAapi::TFileSystemHolder> fileSystem = InitHg(pathInfo, commitInfo, storePath, server);
    Cerr << "Mount " << src << "@" << cs << " to " << dest << Endl;

    RunFuse(argv[0], dest, fileSystem);
    return 0;
}

int PrintUsage() {
    TString usage("Usage: vcs [hg-id,(svn-head|hg-heads),(svn-commit|hg-changeset)-info,(svn|hg)-path-info,(svn|hg)-list,(svn|hg)-export,(svn|hg)-fuse-mount] [args]");
    Cerr << usage << Endl;
    return 1;
}

int main(int argc, char** argv) {
    if (argc == 1) {
        return PrintUsage();
    }

    try {
        const TString subcommand(argv[1]);
        if (subcommand == "hg-id") {
            return HgId(argc - 1, &argv[1]);
        } else if (subcommand == "svn-head") {
            return SvnHead(argc - 1, &argv[1]);
        } else if (subcommand == "svn-commit-info") {
            return SvnCommitInfo(argc - 1, &argv[1]);
        } else if (subcommand == "svn-path-info") {
            return SvnPathInfo(argc - 1, &argv[1]);
        } else if (subcommand == "svn-list") {
            return SvnList(argc - 1, &argv[1]);
        } else if (subcommand == "svn-export") {
            return SvnExport(argc - 1, &argv[1]);
        } else if (subcommand == "svn-fuse-mount") {
            return SvnFuseMount(argc - 1, &argv[1]);
        } else if (subcommand == "hg-heads") {
            return HgHeads(argc - 1, &argv[1]);
        } else if (subcommand == "hg-changeset-info") {
            return HgChangesetInfo(argc - 1, &argv[1]);
        } else if (subcommand == "hg-list") {
            return HgList(argc - 1, &argv[1]);
        } else if (subcommand == "hg-path-info") {
            return HgPathInfo(argc - 1, &argv[1]);
        } else if (subcommand == "hg-export") {
            return HgExport(argc - 1, &argv[1]);
        } else if (subcommand == "hg-fuse-mount") {
            return HgFuseMount(argc - 1, &argv[1]);
        } else {
            return PrintUsage();
        }
    } catch (const NAapi::TGrpcError& ex) {
        Cerr << CurrentExceptionMessage()
             << " code: " << int(ex.Code) << " message: " << ex.Message << Endl;
        return -1;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return -1;
    }
}

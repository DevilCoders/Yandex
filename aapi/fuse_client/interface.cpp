#include "interface.h"

#include <library/cpp/json/json_writer.h>

#include <util/string/hex.h>

static TString CommitInfoToJson(const NAapi::TPathInfo& path, const NAapi::TSvnCommitInfo& commitInfo) {
    NJson::TJsonValue val(NJson::JSON_MAP);
    val["date"] = ToString(commitInfo.Date);
    val["last_revision"] = ToString(commitInfo.Revision);
    val["author"] = commitInfo.Author;
    val["revision"] = ToString(commitInfo.Revision);
    val["repository"] = TString("svn://arcadia.yandex.ru/arc") + path.Path;
    val["repository_vcs"] = "subversion";
    return WriteJson(val);
}

static TString CommitInfoToJson(const NAapi::TPathInfo& path, const NAapi::THgChangesetInfo& commitInfo) {
    NJson::TJsonValue val(NJson::JSON_MAP);
    TString cs(HexEncode(commitInfo.Hash));
    cs.to_lower();
    val["date"] = ToString(commitInfo.Date);
    val["author"] = commitInfo.Author;
    val["revision"] = "0";
    val["hash"] = cs;
    val["branch"] = commitInfo.Branch;
    val["repository"] = TString("ssh://arcadia-hg.yandex-team.ru/arcadia.hg") + path.Path;
    val["repository_vcs"] = "mercurial";
    return WriteJson(val);
}

THolder<NAapi::TFileSystemHolder> InitSvn(const NAapi::TPathInfo& path, const NAapi::TSvnCommitInfo& commitInfo, const TString& discStore, const TString& proxyAddr) {
    THolder<NAapi::TFileSystemHolder> fileSystem = MakeHolder<NAapi::TFileSystemHolder>(discStore, proxyAddr);
    fileSystem->AddRoot(path, CommitInfoToJson(path, commitInfo));
    return fileSystem;
}

THolder<NAapi::TFileSystemHolder> InitHg(const NAapi::TPathInfo& path, const NAapi::THgChangesetInfo& commitInfo, const TString& discStore, const TString& proxyAddr) {
    THolder<NAapi::TFileSystemHolder> fileSystem = MakeHolder<NAapi::TFileSystemHolder>(discStore, proxyAddr);
    fileSystem->AddRoot(path, CommitInfoToJson(path, commitInfo));
    return fileSystem;
}

int GetattrCallback(const char *path, struct stat *st) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Getattr(path, st);
}

int ReaddirCallback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Readdir(path, buf, filler, offset, fi);
}

int OpenCallback(const char *path, struct fuse_file_info *fi) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Open(path, fi);
}

int ReadCallback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Read(path, buf, size, offset, fi);
}

int WriteCallback(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Write(path, buf, size, offset, fi);
}

int ReleaseCallback(const char *path, struct fuse_file_info *fi) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Release(path, fi);
}

int ReadlinkCallback(const char *path, char *buf, size_t size) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Readlink(path, buf, size);
}

int ChmodCallback(const char *path, mode_t mode) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Chmod(path, mode);
}

int ChownCallback(const char *path, uid_t uid, gid_t gid) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Chown(path, uid, gid);
}

int UtimensCallback(const char *path, const struct timespec tv[2]) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Utimens(path, tv);
}

int UnlinkCallback(const char *path) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Unlink(path);
}

int MknodCallback(const char *path, mode_t mode, dev_t dev) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Mknod(path, mode, dev);
}

int MkdirCallback(const char *path, mode_t mode) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Mkdir(path, mode);
}

int RmdirCallback(const char *path) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Rmdir(path);
}

int SymlinkCallback(const char *to, const char *path) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Symlink(to, path);
}

int RenameCallback(const char *path, const char *to) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Rename(path, to);
}

int LinkCallback(const char *to, const char *path) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Link(to, path);
}

int TruncateCallback(const char *path, off_t size) {
    return ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Truncate(path, size);
}

void DestroyCallback(void *) {
    ((NAapi::TFileSystemHolder*)fuse_get_context()->private_data)->Destroy();
}

fuse_operations GetAapiFuseOperations() {
    struct fuse_operations fs = {
        .getattr = GetattrCallback,
        .readlink = ReadlinkCallback,
        .mknod = MknodCallback,
        .mkdir = MkdirCallback,
        .unlink = UnlinkCallback,
        .rmdir = RmdirCallback,
        .symlink = SymlinkCallback,
        .rename = RenameCallback,
    //    .link = LinkCallback,
        .chmod = ChmodCallback,
        .chown = ChownCallback,
        .truncate = TruncateCallback,
        .open = OpenCallback,
        .read = ReadCallback,
        .write = WriteCallback,
        .release = ReleaseCallback,
        .readdir = ReaddirCallback,
        .destroy = DestroyCallback,
        .utimens = UtimensCallback,
    };
    return fs;
}

fuse_args CreateFuseArgs(const TVector<TString> args) {
    struct fuse_args fuseArgs = FUSE_ARGS_INIT(0, NULL);
    for (const TString& arg : args) {
        if (fuse_opt_add_arg(&fuseArgs, arg.c_str()) != 0) {
            ythrow yexception() << "Unable to add argument to fuse options, arg = " << arg;
        }
    }
    return fuseArgs;
}

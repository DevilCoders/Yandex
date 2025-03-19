#include "internal.h"

#include <cloud/filestore/gateway/nfs/libs/api/service.h>

#include <contrib/restricted/nfs_ganesha/src/include/FSAL/fsal_commonlib.h>
#include <contrib/restricted/nfs_ganesha/src/include/fsal_convert.h>

////////////////////////////////////////////////////////////////////////////////

struct yfs_fsal_obj_handle *yfs_alloc_handle(
    struct yfs_fsal_export *exp,
    struct yfs_fsal_obj_handle *parent,
    const char *name,
    struct attrlist *attrs)
{
    struct yfs_fsal_obj_handle *hdl = gsh_calloc(1, sizeof(struct yfs_fsal_obj_handle));

    // init node attributes
    if (attrs && FSAL_TEST_MASK(attrs->valid_mask, ATTR_FILEID)) {
        hdl->attrs.fileid = attrs->fileid;
    }

    if (attrs && FSAL_TEST_MASK(attrs->valid_mask, ATTR_TYPE)) {
        hdl->attrs.type = attrs->type;
    }

    if (attrs && FSAL_TEST_MASK(attrs->valid_mask, ATTR_MODE)) {
        hdl->attrs.mode = attrs->mode;
    }

    if (attrs && FSAL_TEST_MASK(attrs->valid_mask, ATTR_OWNER)) {
        hdl->attrs.owner = attrs->owner;
    } else {
        hdl->attrs.owner = op_ctx->creds->caller_uid;
    }

    if (attrs && FSAL_TEST_MASK(attrs->valid_mask, ATTR_GROUP)) {
        hdl->attrs.group = attrs->group;
    } else {
        hdl->attrs.group = op_ctx->creds->caller_gid;
    }

    if (attrs && FSAL_TEST_MASK(attrs->valid_mask, ATTR_CTIME)) {
        hdl->attrs.ctime = attrs->ctime;
    } else {
        now(&hdl->attrs.ctime);
    }

    if (attrs && FSAL_TEST_MASK(attrs->valid_mask, ATTR_ATIME)) {
        hdl->attrs.atime = attrs->atime;
    } else {
        hdl->attrs.atime = hdl->attrs.ctime;
    }

    if (attrs && FSAL_TEST_MASK(attrs->valid_mask, ATTR_MTIME)) {
        hdl->attrs.mtime = attrs->mtime;
    } else {
        hdl->attrs.mtime = hdl->attrs.ctime;
    }

    hdl->attrs.change = timespec_to_nsecs(&hdl->attrs.ctime);

    switch (hdl->attrs.fileid) {
    case REGULAR_FILE:
        if (attrs && FSAL_TEST_MASK(attrs->valid_mask, ATTR_SIZE)) {
            hdl->attrs.filesize = attrs->filesize;
            hdl->attrs.spaceused = attrs->filesize;
        } else {
            hdl->attrs.filesize = 0;
            hdl->attrs.spaceused = 0;
        }
        hdl->attrs.numlinks = 1;
        break;

    case BLOCK_FILE:
    case CHARACTER_FILE:
        if (attrs && FSAL_TEST_MASK(attrs->valid_mask, ATTR_RAWDEV)) {
            hdl->attrs.rawdev.major = attrs->rawdev.major;
            hdl->attrs.rawdev.minor = attrs->rawdev.minor;
        } else {
            hdl->attrs.rawdev.major = 0;
            hdl->attrs.rawdev.minor = 0;
        }
        hdl->attrs.numlinks = 1;
        break;

    case DIRECTORY:
        hdl->attrs.numlinks = 2;
        break;

    default:
        hdl->attrs.numlinks = 1;
        break;
    }

    /* Need an FSID */
    hdl->attrs.fsid.major = op_ctx->ctx_export->export_id;;
    hdl->attrs.fsid.minor = 0;

    /* Set the mask at the end. */
    hdl->attrs.valid_mask = ATTRS_POSIX;
    hdl->attrs.supported = ATTRS_POSIX;

    LogDebug(COMPONENT_FSAL, "Allocating hdl=0x%p, name=%s",
        &hdl->obj_handle, name);

    fsal_obj_handle_init(&hdl->obj_handle, &exp->export, hdl->attrs.type);
    hdl->obj_handle.obj_ops = &exp->handle_ops;

    hdl->obj_handle.fileid = attrs->fileid;
    hdl->obj_handle.fsid.major = hdl->attrs.fsid.major;
    hdl->obj_handle.fsid.minor = hdl->attrs.fsid.minor;

    hdl->parent = &parent->obj_handle;
    hdl->name = gsh_strdup(name);

    // TODO
    hdl->host_handle.handle = 0;
    hdl->host_handle.ino = attrs->fileid;

    return hdl;
}

void yfs_free_handle(struct yfs_fsal_obj_handle *hdl)
{
    LogDebug(COMPONENT_FSAL, "Releasing hdl=0x%p", &hdl->obj_handle);

    fsal_obj_handle_fini(&hdl->obj_handle);

    gsh_free(hdl->name);
    gsh_free(hdl);
}

static void yfs_release_handle(struct fsal_obj_handle *obj_hdl)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);

    if (hdl != exp->root_handle) {
        yfs_free_handle(hdl);
    }
}

static fsal_status_t yfs_merge_handle(
    struct fsal_obj_handle *old_hdl,
    struct fsal_obj_handle *new_hdl)
{
    fsal_status_t status = {ERR_FSAL_NO_ERROR, 0};

    if (old_hdl == new_hdl) {
        /* Nothing to merge */
        return status;
    }

    if (old_hdl->type == REGULAR_FILE &&
        new_hdl->type == REGULAR_FILE) {
        /* We need to merge the share reservations on this file.
         * This could result in ERR_FSAL_SHARE_DENIED.
         */
        struct yfs_fsal_obj_handle *old = YFS_HANDLE_FROM_FSAL(old_hdl);
        struct yfs_fsal_obj_handle *new = YFS_HANDLE_FROM_FSAL(new_hdl);

        /* This can block over an I/O operation. */
        PTHREAD_RWLOCK_wrlock(&old_hdl->obj_lock);
        status = merge_share(&old->share, &new->share);
        PTHREAD_RWLOCK_unlock(&old_hdl->obj_lock);
    }

    return status;
}

static fsal_status_t yfs_handle_to_wire(
    const struct fsal_obj_handle *obj_hdl,
    fsal_digesttype_t output_type,
    struct gsh_buffdesc *fh_desc)
{
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);
    struct yfs_fsal_host_handle *hhdl = fh_desc->addr;

    switch (output_type) {
        case FSAL_DIGEST_NFSV3:
        case FSAL_DIGEST_NFSV4:
            if (fh_desc->len < sizeof(struct yfs_fsal_host_handle)) {
                LogMajor(COMPONENT_FSAL,
                    "Space too small for handle. Need %zu, have %zu",
                    sizeof(struct yfs_fsal_host_handle),
                    fh_desc->len);
                return fsalstat(ERR_FSAL_TOOSMALL, 0);
            }

            fh_desc->len = sizeof(struct yfs_fsal_host_handle);

            // convert byte order to wire
            hhdl->handle = htole64(hdl->host_handle.handle);
            hhdl->ino = htole64(hdl->host_handle.ino);
            break;

        default:
            return fsalstat(ERR_FSAL_SERVERFAULT, 0);
    }

    return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

static void yfs_handle_to_key(
    struct fsal_obj_handle *obj_hdl,
    struct gsh_buffdesc *fh_desc)
{
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);

    fh_desc->addr = &hdl->host_handle;
    fh_desc->len = sizeof(struct yfs_fsal_host_handle);
}

static fsal_status_t yfs_lookup(
    struct fsal_obj_handle *dir_hdl,
    const char *name,
    struct fsal_obj_handle **obj_hdl,
    struct attrlist *attrs_out)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *dir = YFS_HANDLE_FROM_FSAL(dir_hdl);

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    LogDebug(COMPONENT_FSAL, "Lookup hdl=0x%p, name=%s", dir_hdl, name);

    if (strcmp(name, ".") == 0) {
        // lookup self
        *obj_hdl = dir_hdl;
        return fsalstat(ERR_FSAL_NO_ERROR, 0);
    }

    if (strcmp(name, "..") == 0) {
        // lookup parent
        if (dir->parent != NULL) {
            *obj_hdl = dir->parent;
            return fsalstat(ERR_FSAL_NO_ERROR, 0);
        }
        return fsalstat(ERR_FSAL_NOENT, 0);
    }

    // lookup child
    struct stat st = {};
    retval = YFS_CALL(lookup,
        exp->service,
        dir_hdl->fileid,
        name,
        &st);

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
        goto out;
    }

    struct attrlist attrs;
    fsal_prepare_attrs(&attrs, ATTRS_POSIX);

    posix2fsal_attributes_all(&st, &attrs);

    struct yfs_fsal_obj_handle *hdl = yfs_alloc_handle(exp, dir, name, &attrs);
    *obj_hdl = &hdl->obj_handle;

    if (attrs_out != NULL) {
        fsal_copy_attrs(attrs_out, &hdl->attrs, false);
    }

    fsal_release_attrs(&attrs);

out:
    return fsalstat(fsal_error, retval);
}

struct yfs_readdir_state {
    struct yfs_readdir_cb cb;
    fsal_readdir_cb readdir_cb;
    void *readdir_state;
    struct yfs_fsal_obj_handle *dir;
    attrmask_t attrmask;
};

static int yfs_readdir_callback(
    struct yfs_readdir_cb *cb,
    const char *name,
    const struct stat *st,
    uint64_t offset)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_readdir_state *state = container_of(cb, struct yfs_readdir_state, cb);

    struct attrlist attrs;
    fsal_prepare_attrs(&attrs, state->attrmask);

    posix2fsal_attributes_all(st, &attrs);

    struct yfs_fsal_obj_handle *hdl = yfs_alloc_handle(exp, state->dir, name, &attrs);

    // TODO
    int retval = state->readdir_cb(
        name,
        &hdl->obj_handle,
        &attrs,
        state->readdir_state,
        offset);

    fsal_release_attrs(&attrs);
    return 0;
}

static fsal_status_t yfs_readdir(
    struct fsal_obj_handle *dir_hdl,
    fsal_cookie_t *whence,
    void *dir_state,
    fsal_readdir_cb cb,
    attrmask_t attrmask,
    bool *eof)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *dir = YFS_HANDLE_FROM_FSAL(dir_hdl);

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    fsal_cookie_t offset = 0;
    if (whence != NULL) {
        offset = *whence;
    }

    LogDebug(COMPONENT_FSAL, "ReadDir hdl=0x%p, name=%s, offset=%lu",
        dir_hdl, dir->name, offset);

    *eof = true;

    void *handle = NULL;
    retval = YFS_CALL(opendir, exp->service, dir_hdl->fileid, &handle);

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
        goto out;
    }

    struct yfs_readdir_state state = {
        .cb.invoke = yfs_readdir_callback,
        .readdir_cb = cb,
        .readdir_state = dir_state,
        .dir = dir,
        .attrmask = attrmask,
    };

    for (;;) {
        retval = YFS_CALL(readdir,
            exp->service,
            handle,
            &state.cb,
            offset);

        if (retval <= 0) {
            break;
        }

        offset += retval;
    }

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
    }

    // TODO
    YFS_CALL(releasedir, exp->service, handle);

out:
    return fsalstat(fsal_error, retval);
}

static fsal_status_t yfs_mkdir(
    struct fsal_obj_handle *dir_hdl,
    const char *name,
    struct attrlist *attrs_in,
    struct fsal_obj_handle **new_obj,
    struct attrlist *attrs_out)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *dir = YFS_HANDLE_FROM_FSAL(dir_hdl);

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    LogDebug(COMPONENT_FSAL, "MkDir hdl=0x%p, name=%s", dir_hdl, name);

    *new_obj = NULL;
    if (!fsal_obj_handle_is(dir_hdl, DIRECTORY)) {
        LogCrit(COMPONENT_FSAL, "Parent handle is not a directory. hdl=0x%p",
            dir_hdl);
        return fsalstat(ERR_FSAL_NOTDIR, 0);
    }

    mode_t unix_mode = fsal2unix_mode(attrs_in->mode)
        & ~exp->export.exp_ops.fs_umask(&exp->export);

    struct stat st = {};
    retval = YFS_CALL(mkdir,
        exp->service,
        dir_hdl->fileid,
        name,
        unix_mode,
        &st);

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
        goto out;
    }

    struct attrlist attrs;
    fsal_prepare_attrs(&attrs, ATTRS_POSIX);

    posix2fsal_attributes_all(&st, &attrs);

    struct yfs_fsal_obj_handle *hdl = yfs_alloc_handle(exp, dir, name, &attrs);
    *new_obj = &hdl->obj_handle;

    if (attrs_out != NULL) {
        fsal_copy_attrs(attrs_out, &hdl->attrs, false);
    }

out:
    return fsalstat(fsal_error, retval);
}

static fsal_status_t yfs_mknode(
    struct fsal_obj_handle *dir_hdl,
    const char *name,
    object_file_type_t nodetype,
    struct attrlist *attrs_in,
    struct fsal_obj_handle **new_obj,
    struct attrlist *attrs_out)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *dir = YFS_HANDLE_FROM_FSAL(dir_hdl);

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    LogDebug(COMPONENT_FSAL, "MkNode hdl=0x%p, name=%s", dir_hdl, name);

    *new_obj = NULL;
    if (!fsal_obj_handle_is(dir_hdl, DIRECTORY)) {
        LogCrit(COMPONENT_FSAL, "Parent handle is not a directory. hdl=0x%p",
            dir_hdl);
        return fsalstat(ERR_FSAL_NOTDIR, 0);
    }

    mode_t unix_mode = fsal2unix_mode(attrs_in->mode)
        & ~exp->export.exp_ops.fs_umask(&exp->export);

    dev_t unix_dev = 0;
    switch (nodetype) {
    case BLOCK_FILE:
        unix_mode |= S_IFBLK;
        unix_dev = makedev(attrs_in->rawdev.major, attrs_in->rawdev.minor);
        break;
    case CHARACTER_FILE:
        unix_mode |= S_IFCHR;
        unix_dev = makedev(attrs_in->rawdev.major, attrs_in->rawdev.minor);
        break;
    case FIFO_FILE:
        unix_mode |= S_IFIFO;
        break;
    case SOCKET_FILE:
        unix_mode |= S_IFSOCK;
        break;
    default:
        LogMajor(COMPONENT_FSAL, "Invalid node type in FSAL_mknode: %d",
            nodetype);
        fsal_error = ERR_FSAL_INVAL;
        goto out;
    }

    struct stat st = {};
    retval = YFS_CALL(mknode,
        exp->service,
        dir_hdl->fileid,
        name,
        unix_mode,
        unix_dev,
        &st);

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
        goto out;
    }

    struct attrlist attrs;
    fsal_prepare_attrs(&attrs, ATTRS_POSIX);

    posix2fsal_attributes_all(&st, &attrs);

    struct yfs_fsal_obj_handle *hdl = yfs_alloc_handle(exp, dir, name, &attrs);
    *new_obj = &hdl->obj_handle;

    if (attrs_out != NULL) {
        fsal_copy_attrs(attrs_out, &hdl->attrs, false);
    }

out:
    return fsalstat(fsal_error, retval);
}

static fsal_status_t yfs_symlink(
    struct fsal_obj_handle *dir_hdl,
    const char *name,
    const char *link_path,
    struct attrlist *attrs_in,
    struct fsal_obj_handle **new_obj,
    struct attrlist *attrs_out)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *dir = YFS_HANDLE_FROM_FSAL(dir_hdl);

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    LogDebug(COMPONENT_FSAL, "SymLink hdl=0x%p, name=%s, path=%s",
        dir_hdl, name, link_path);

    *new_obj = NULL;
    if (!fsal_obj_handle_is(dir_hdl, DIRECTORY)) {
        LogCrit(COMPONENT_FSAL, "Parent handle is not a directory. hdl=0x%p",
            dir_hdl);
        return fsalstat(ERR_FSAL_NOTDIR, 0);
    }

    struct stat st = {};
    retval = YFS_CALL(symlink,
        exp->service,
        dir_hdl->fileid,
        name,
        link_path,
        &st);

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
        goto out;
    }

    struct attrlist attrs;
    fsal_prepare_attrs(&attrs, ATTRS_POSIX);

    posix2fsal_attributes_all(&st, &attrs);

    struct yfs_fsal_obj_handle *hdl = yfs_alloc_handle(exp, dir, name, &attrs);
    *new_obj = &hdl->obj_handle;

    if (attrs_out != NULL) {
        fsal_copy_attrs(attrs_out, &hdl->attrs, false);
    }

out:
    return fsalstat(fsal_error, retval);
}

static fsal_status_t yfs_readlink(
    struct fsal_obj_handle *obj_hdl,
    struct gsh_buffdesc *link_content,
    bool refresh)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);

    LogDebug(COMPONENT_FSAL, "ReadLink hdl=0x%p", obj_hdl);

    // TODO
    printf("yfs_readlink\n");
    return fsalstat(ERR_FSAL_NOTSUPP, 0);
}

static fsal_status_t yfs_getattrs(
    struct fsal_obj_handle *obj_hdl,
    struct attrlist *attrs_out)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    LogDebug(COMPONENT_FSAL, "GetAttrs hdl=0x%p", obj_hdl);

    struct stat st = {};
    retval = YFS_CALL(getattr, exp->service, obj_hdl->fileid, &st);

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
        goto out;
    }

    if (attrs_out != NULL) {
        posix2fsal_attributes_all(&st, attrs_out);
    }

out:
    return fsalstat(fsal_error, retval);
}

static fsal_status_t yfs_setattr2(
    struct fsal_obj_handle *obj_hdl,
    bool bypass,
    struct state_t *state,
    struct attrlist *attrs_in)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    LogDebug(COMPONENT_FSAL, "SetAttr hdl=0x%p", obj_hdl);

    struct stat st = {};
    uint32_t flags = 0;

    if (FSAL_TEST_MASK(attrs_in->valid_mask, ATTR_MODE)) {
        st.st_mode = fsal2unix_mode(attrs_in->mode);
        flags |= F_SET_ATTR_MODE;
    }
    if (FSAL_TEST_MASK(attrs_in->valid_mask, ATTR_OWNER)) {
        st.st_uid = attrs_in->owner;
        flags |= F_SET_ATTR_UID;
    }
    if (FSAL_TEST_MASK(attrs_in->valid_mask, ATTR_GROUP)) {
        st.st_gid = attrs_in->group;
        flags |= F_SET_ATTR_GID;
    }
    if (FSAL_TEST_MASK(attrs_in->valid_mask, ATTR_SIZE)) {
        st.st_size = attrs_in->filesize;
        flags |= F_SET_ATTR_SIZE;
    }
    if (FSAL_TEST_MASK(attrs_in->valid_mask, ATTR_ATIME)) {
        st.st_atim = attrs_in->atime;
        flags |= F_SET_ATTR_ATIME;
    }
    if (FSAL_TEST_MASK(attrs_in->valid_mask, ATTR_MTIME)) {
        st.st_mtim = attrs_in->mtime;
        flags |= F_SET_ATTR_MTIME;
    }

    retval = YFS_CALL(setattr, exp->service, obj_hdl->fileid, &st, flags);

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
        goto out;
    }

out:
    return fsalstat(fsal_error, retval);
}

static fsal_status_t yfs_link(
    struct fsal_obj_handle *obj_hdl,
    struct fsal_obj_handle *dir_hdl,
    const char *name)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);
    struct yfs_fsal_obj_handle *dir = YFS_HANDLE_FROM_FSAL(dir_hdl);

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    LogDebug(COMPONENT_FSAL, "Link hdl=0x%p, dir=0x%p, name=%s",
        obj_hdl, dir_hdl, name);

    struct stat st = {};
    retval = YFS_CALL(link,
        exp->service,
        dir_hdl->fileid,
        name,
        obj_hdl->fileid,
        &st);

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
        goto out;
    }

    // TODO

out:
    return fsalstat(fsal_error, retval);
}

static fsal_status_t yfs_unlink(
    struct fsal_obj_handle *dir_hdl,
    struct fsal_obj_handle *obj_hdl,
    const char *name)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);
    struct yfs_fsal_obj_handle *dir = YFS_HANDLE_FROM_FSAL(dir_hdl);

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    LogDebug(COMPONENT_FSAL, "Unlink hdl=0x%p, dir=0x%p, name=%s",
        obj_hdl, dir_hdl, name);

    retval = YFS_CALL(unlink, exp->service, dir_hdl->fileid, name);

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
        goto out;
    }

    // TODO

out:
    return fsalstat(fsal_error, retval);
}

static fsal_status_t yfs_rename(
    struct fsal_obj_handle *obj_hdl,
    struct fsal_obj_handle *olddir_hdl,
    const char *old_name,
    struct fsal_obj_handle *newdir_hdl,
    const char *new_name)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);
    struct yfs_fsal_obj_handle *olddir = YFS_HANDLE_FROM_FSAL(olddir_hdl);
    struct yfs_fsal_obj_handle *newdir = YFS_HANDLE_FROM_FSAL(newdir_hdl);

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    LogDebug(COMPONENT_FSAL,
        "Rename hdl=0x%p, olddir=0x%p, oldname=%s, newdir=0x%p, newname=%s",
        obj_hdl, olddir_hdl, old_name, newdir_hdl, new_name);

    retval = YFS_CALL(rename,
        exp->service,
        olddir_hdl->fileid,
        old_name,
        newdir_hdl->fileid,
        new_name);

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
        goto out;
    }

    // TODO

out:
    return fsalstat(fsal_error, retval);
}

static fsal_status_t yfs_open2(
    struct fsal_obj_handle *obj_hdl,
    struct state_t *state,
    fsal_openflags_t openflags,
    enum fsal_create_mode createmode,
    const char *name,
    struct attrlist *attrs_in,
    fsal_verifier_t verifier,
    struct fsal_obj_handle **new_obj,
    struct attrlist *attrs_out,
    bool *caller_perm_check)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);

    LogDebug(COMPONENT_FSAL, "Open");

    // TODO
    printf("yfs_open2\n");
    return fsalstat(ERR_FSAL_NOTSUPP, 0);
}

static fsal_status_t yfs_reopen2(
    struct fsal_obj_handle *obj_hdl,
    struct state_t *state,
    fsal_openflags_t openflags)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);

    LogDebug(COMPONENT_FSAL, "ReOpen");

    // TODO
    printf("yfs_reopen2\n");
    return fsalstat(ERR_FSAL_NOTSUPP, 0);
}

static fsal_status_t yfs_close(struct fsal_obj_handle *obj_hdl)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);

    LogDebug(COMPONENT_FSAL, "Close");

    // TODO
    printf("yfs_close\n");
    return fsalstat(ERR_FSAL_NOTSUPP, 0);
}

static fsal_status_t yfs_close2(
    struct fsal_obj_handle *obj_hdl,
    struct state_t *state)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);

    LogDebug(COMPONENT_FSAL, "Close2");

    // TODO
    printf("yfs_close2\n");
    return fsalstat(ERR_FSAL_NOTSUPP, 0);
}

static void yfs_read2(
    struct fsal_obj_handle *obj_hdl,
    bool bypass,
    fsal_async_cb done_cb,
    struct fsal_io_arg *read_arg,
    void *caller_arg)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);

    LogDebug(COMPONENT_FSAL, "Read");

    // TODO
    printf("yfs_read2\n");
}

static void yfs_write2(
    struct fsal_obj_handle *obj_hdl,
    bool bypass,
    fsal_async_cb done_cb,
    struct fsal_io_arg *write_arg,
    void *caller_arg)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);

    LogDebug(COMPONENT_FSAL, "Write");

    // TODO
    printf("yfs_write2\n");
}

static fsal_status_t yfs_lock_op2(
    struct fsal_obj_handle *obj_hdl,
    struct state_t *state,
    void *owner,
    fsal_lock_op_t lock_op,
    fsal_lock_param_t *request_lock,
    fsal_lock_param_t *conflicting_lock)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(op_ctx->fsal_export);
    struct yfs_fsal_obj_handle *hdl = YFS_HANDLE_FROM_FSAL(obj_hdl);

    LogDebug(COMPONENT_FSAL, "Lock");

    // TODO
    printf("yfs_lock_op2\n");
    return fsalstat(ERR_FSAL_NOTSUPP, 0);
}

void yfs_handle_ops_init(struct fsal_obj_ops *ops)
{
    fsal_default_obj_ops_init(ops);

    ops->release = yfs_release_handle;
    ops->merge = yfs_merge_handle,
    ops->handle_to_wire = yfs_handle_to_wire;
    ops->handle_to_key = yfs_handle_to_key;
    ops->lookup = yfs_lookup;
    ops->readdir = yfs_readdir;
    ops->mkdir = yfs_mkdir;
    ops->mknode = yfs_mknode;
    ops->symlink = yfs_symlink;
    ops->readlink = yfs_readlink;
    ops->getattrs = yfs_getattrs;
    ops->setattr2 = yfs_setattr2;
    ops->link = yfs_link;
    ops->unlink = yfs_unlink;
    ops->rename = yfs_rename;
    ops->open2 = yfs_open2;
    ops->reopen2 = yfs_reopen2;
    ops->close = yfs_close;
    ops->close2 = yfs_close2;
    ops->read2 = yfs_read2;
    ops->write2 = yfs_write2;
    ops->lock_op2 = yfs_lock_op2;
}

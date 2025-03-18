import os
import stat
import threading
import hashlib
import vcs_helpers
import concurrent.futures
import consts
import struct
import uuid


def mode(path):
    if os.path.islink(path):
        return consts.MODE_LINK
    if os.path.isdir(path):
        return consts.MODE_DIR
    st = os.stat(path).st_mode
    if st & (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH):
        return consts.MODE_EXEC
    return consts.MODE_REG


class Store2(object):
    _HEX = '0123456789abcdef'

    def __init__(self, path):
        self._path = path
        if not os.path.exists(path):
            os.makedirs(path)
        for x in self._HEX:
            for y in self._HEX:
                p = os.path.join(path, x + y)
                if not os.path.exists(p):
                    os.makedirs(p)

    def inner_path(self, key_bin):
        key = vcs_helpers.hexdump(key_bin)
        return os.path.join(self._path, key[:2], key)

    def put(self, key_bin, _type, data):
        path = self.inner_path(key_bin)
        tmp_path = path + str(uuid.uuid4())

        with open(tmp_path, 'wb') as f:
            f.write(struct.pack("B", _type))
            f.write(data)

        os.rename(tmp_path, path)

    def get(self, key_bin):
        if not os.path.exists(self.inner_path(key_bin)):
            raise KeyError(key_bin)

        with open(self.inner_path(key_bin), 'rb') as f:
            _type = struct.unpack("B", f.read(1))[0]
            return _type, f.read()

    def has(self, key_bin):
        return os.path.exists(self.inner_path(key_bin))


class Store3(object):
    _HEX = '0123456789abcdef'

    def __init__(self, path):
        self._path = path
        if not os.path.exists(path):
            os.makedirs(path)
        for x in self._HEX:
            for y in self._HEX:
                for z in self._HEX:
                    for w in self._HEX:
                        p = os.path.join(path, x, y, z, w)
                        if not os.path.exists(p):
                            os.makedirs(p)

    def inner_path(self, key_bin):
        key = vcs_helpers.hexdump(key_bin)
        return os.path.join(self._path, key[0], key[1], key[2], key[3], key)

    def put(self, key_bin, _type, data):
        path = self.inner_path(key_bin)
        tmp_path = path + str(uuid.uuid4())

        with open(tmp_path, 'wb') as f:
            f.write(struct.pack("B", _type))
            f.write(data)

        os.rename(tmp_path, path)

    def get(self, key_bin):
        if not self.has(key_bin):
            raise KeyError(key_bin.encode('hex'))

        with open(self.inner_path(key_bin), 'rb') as f:
            _type = struct.unpack("B", f.read(1))[0]
            return _type, f.read()

    def has(self, key_bin):
        return os.path.exists(self.inner_path(key_bin))


class Store(object):
    _HEX = '0123456789abcdef'

    def __init__(self, path):
        self._path = path
        if not os.path.exists(path):
            os.makedirs(path)
        for x in self._HEX:
            for y in self._HEX:
                p = os.path.join(path, x + y)
                if not os.path.exists(p):
                    os.makedirs(p)

    def path(self):
        return self._path

    def put(self, s):
        h = vcs_helpers.git_like_hash(s)
        p = os.path.join(self._path, h.hexdigest()[:2], h.hexdigest())
        tp = p + str(threading.current_thread().ident)
        with open(tp, 'wb') as f:
            f.write(s)
        os.rename(tp, p)
        return h.digest()

    def has(self, sha):
        sha_s = vcs_helpers.hexdump(sha)
        assert len(sha) == 20 and len(sha_s) == 40
        return os.path.exists(os.path.join(self._path, sha_s[:2], sha_s))

    def get(self, sha):
        sha_s = vcs_helpers.hexdump(sha)
        assert len(sha) == 20 and len(sha_s) == 40
        with open(os.path.join(self._path, sha_s[:2], sha_s), 'rb') as f:
            return f.read()


class ObjectStore(object):

    def __init__(self, path):
        self._path = path
        self._blob_store = Store(os.path.join(path, 'blob'))
        self._tree_store = Store(os.path.join(path, 'tree'))
        self._svn_ci_store = Store(os.path.join(path, 'svn_ci'))
        self._head = os.path.join(path, 'SVN_HEAD')

    def blob_store(self):
        return self._blob_store

    def tree_store(self):
        return self._tree_store

    def svn_ci_store(self):
        return self._svn_ci_store

    def head_path(self):
        return self._head

    def put_raw_object(self, _type, data):
        assert len(data) <= consts.YT_CHUNK
        if _type == consts.NODE_BLOB:
            return self._blob_store.put(data)
        elif _type == consts.NODE_TREE:
            return self._tree_store.put(data)
        elif _type == consts.NODE_SVN_CI:
            return self._svn_ci_store.put(data)
        elif _type == consts.NODE_SVN_HEAD:
            assert len(data) == 20
            with open(self._head, 'wb') as f:
                f.write(data)
            return 'SVN_HEAD'
        else:
            raise Exception('Unknown type {}'.format(_type))

    def put_object(self, _type, obj):
        if _type == consts.NODE_BLOB:
            data = vcs_helpers.compress(obj)
            assert len(data) <= consts.YT_CHUNK
            return self._blob_store.put(data)
        elif _type == consts.NODE_TREE:
            data = vcs_helpers.compress(vcs_helpers.dump_tree_fbs(obj))
            assert len(data) <= consts.YT_CHUNK
            return self._tree_store.put(data)
        elif _type == consts.NODE_SVN_CI:
            data = vcs_helpers.compress(vcs_helpers.dump_svn_commit_fbs(obj))
            assert len(data) <= consts.YT_CHUNK
            return self._svn_ci_store.put(data)
        elif _type == consts.NODE_SVN_HEAD:
            data = obj
            assert len(data) == 20
            with open(self._head, 'wb') as f:
                f.write(data)
            return 'SVN_HEAD'
        else:
            raise Exception('Unknown type {}'.format(_type))

    def has(self, k):
        if k == 'SVN_HEAD':
            return os.path.exists(self._head)
        else:
            return self._blob_store.has(k) or self._tree_store.has(k) or self._svn_ci_store.has(k)

    def get_type_and_raw_object(self, k):
        if k == 'SVN_HEAD':
            with open(self._head, 'rb') as f:
                return consts.NODE_SVN_HEAD, f.read()
        elif self._blob_store.has(k):
            return consts.NODE_BLOB, self._blob_store.get(k)
        elif self._tree_store.has(k):
            return consts.NODE_TREE, self._tree_store.get(k)
        elif self._svn_ci_store.has(k):
            return consts.NODE_SVN_CI, self._svn_ci_store.get(k)
        else:
            raise Exception('No such object {}'.format(vcs_helpers.hexdump(k)))

    def get_type_and_object(self, k):
        _type, data = self.get_type_and_raw_object(k)
        if _type == consts.NODE_SVN_HEAD:
            obj = data
        elif _type == consts.NODE_BLOB:
            obj = vcs_helpers.decompress(data)
        elif _type == consts.NODE_TREE:
            obj = vcs_helpers.load_tree_fbs(vcs_helpers.decompress(data))
        elif _type == consts.NODE_SVN_CI:
            obj = vcs_helpers.load_svn_commit_fbs(vcs_helpers.decompress(data))
        else:
            raise Exception('Unknown type {}'.format(_type))
        return _type, obj

    def get_raw_object(self, k):
        _type, data = self.get_type_and_raw_object(k)
        return data

    def get_object(self, k):
        _type, obj = self.get_type_and_object(k)
        return obj

    def get_type(self, k):
        _type, obj = self.get_type_and_object(k)
        return _type


class FileStore(object):

    def __init__(self, store, threads=32):
        self._store = store
        self._pool = concurrent.futures.ThreadPoolExecutor(max_workers=threads)

    def _put_file(self, path):
        m = mode(path)
        assert m in (consts.MODE_REG, consts.MODE_EXEC)
        blobs = []
        sha = hashlib.sha1()
        size = 0

        with open(path, 'rb') as f:
            while True:
                data = f.read(consts.YT_CHUNK - 4 * 1024)

                if not data:
                    break

                compressed_data = vcs_helpers.compress(data)
                assert len(compressed_data) <= consts.YT_CHUNK
                blobs.append(self._store.put(compressed_data))
                sha.update(data)
                size += len(data)

        if size == 0:
            blobs.append(self._store.put(vcs_helpers.compress('')))
            sha.update('')

        sha.update('\0' + str(size))
        return vcs_helpers.TreeEntry(os.path.basename(path), m, sha.digest(), blobs)

    def _put_link(self, path):
        m = mode(path)
        assert m == consts.MODE_LINK
        s = os.readlink(path)
        blobs = [self._store.put(vcs_helpers.compress(s))]
        return vcs_helpers.TreeEntry(os.path.basename(path), m, vcs_helpers.git_like_hash(s).digest(), blobs)

    def put_file(self, path):
        m = mode(path)
        assert m in (consts.MODE_REG, consts.MODE_EXEC, consts.MODE_LINK)
        if m in (consts.MODE_REG, consts.MODE_EXEC):
            return self._put_file(path)
        else:
            return self._put_link(path)

    def put_file_future(self, path):
        return self._pool.submit(self.put_file, path).result


class TreeStore(object):

    def __init__(self, store):
        self._store = store

    def put_tree(self, path, entries):
        m = mode(path)
        assert m == consts.MODE_DIR
        return self.put_tree_nocheck(os.path.basename(path), entries)

    def put_tree_nocheck(self, basename, entries):
        entries.sort(key=lambda e: (e.mode != consts.MODE_DIR, e.name))
        tree = vcs_helpers.Tree(entries)
        tree_hash = self._store.put(vcs_helpers.compress(vcs_helpers.dump_tree_fbs(tree)))
        return vcs_helpers.TreeEntry(basename, consts.MODE_DIR, tree_hash, [])


class SvnCommitStore(object):

    def __init__(self, store):
        self._store = store

    def put_svn_commit(self, ci):
        assert isinstance(ci, vcs_helpers.SvnCommit)
        return self._store.put(vcs_helpers.compress(vcs_helpers.dump_svn_commit_fbs(ci)))

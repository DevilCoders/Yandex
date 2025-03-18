import sys
import vcs_helpers
import consts


class NoSuchFile(Exception):
    pass


class NoSuchObject(Exception):
    pass


class Vcs(object):

    def __init__(self, table, client, object_store):
        self._table = table
        self._client = client
        self._store = object_store

    def get_object(self, sha, update=False):
        if update or not self._store.has(sha):
            rows = self._client.lookup_rows(self._table, [{'hash': sha}], columns=['type', 'data'])
            assert len(rows) < 2
            if len(rows) == 0:
                raise NoSuchObject('No such object: {}'.format(vcs_helpers.hexdump(sha)))
            _type, data = rows[0]['type'], rows[0]['data']
            self._store.put_raw_object(_type, data)
        assert self._store.has(sha)
        return self._store.get_type_and_object(sha)

    def put_object(self, _type, obj, upload=True):
        sha = self._store.put_object(_type, obj)
        data = self._store.get_raw_object(sha)
        if upload:
            self._client.insert_rows(self._table,  [{'hash': sha, 'type': _type, 'data': data}], update=True)
        return sha

    def svn_head(self, update=False):
        _type, sha = self.get_object('SVN_HEAD', update=update)
        assert _type == consts.NODE_SVN_HEAD
        assert len(sha) == 20
        _type, ci = self.get_object(sha)
        assert _type == consts.NODE_SVN_CI
        assert isinstance(ci, vcs_helpers.SvnCommit)
        return ci

    def svn_log(self, max_commits=None, update=False):
        if max_commits is None:
            max_commits = sys.maxint

        cur = self.svn_head(update=update)
        i = 0

        while cur.parent and i < max_commits:
            yield cur
            assert len(cur.parent) == 20
            _type, parent = self.get_object(cur.parent)
            assert _type == consts.NODE_SVN_CI and isinstance(parent, vcs_helpers.SvnCommit)
            cur = parent
            i += 1

    def seek_entry(self, tree_node, entry_name):
        assert isinstance(tree_node, vcs_helpers.Tree)

        for i, entry in enumerate(tree_node.entries):
            if entry.name != entry_name:
                continue
            return i, entry
        return None, None

    def seek(self, root_node, entry_path):
        assert isinstance(root_node, vcs_helpers.Tree)

        if entry_path == '':
            return vcs_helpers.TreeEntry('', consts.MODE_DIR, self.put_object(consts.NODE_TREE, root_node, upload=False), [])

        parts = entry_path.split('/')
        dirname_parts, basename = parts[:-1], parts[-1]
        cur_tree_node = root_node

        for name in dirname_parts:
            _, entry = self.seek_entry(cur_tree_node, name)
            if entry is None or entry.mode != consts.MODE_DIR:
                raise NoSuchFile('No such file or directory: {}'.format(entry_path))
            _type, cur_tree_node = self.get_object(entry.hash)
            assert _type == consts.NODE_TREE

        _, entry = self.seek_entry(cur_tree_node, basename)
        if entry is None:
            raise NoSuchFile('No such file or directory: {}'.format(entry_path))
        return entry

    def cat_file(self, root_node, path):
        assert isinstance(root_node, vcs_helpers.Tree)

        entry = self.seek(root_node, path)
        blobs = []
        for blob_sha in entry.blobs:
            _type, blob = self.get_object(blob_sha)
            assert _type == consts.NODE_BLOB
            blobs.append(blob)

        return ''.join(blobs)

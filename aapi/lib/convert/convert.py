import os
import logging
import hashlib
import subvertpy.repos as repos

import vcs_helpers
import aapi.lib.py_common.consts as consts
import aapi.lib.py_common.paths_tree as paths_tree


def init(vcs_store, svn_head_file):
    tree = vcs_helpers.Tree([])
    tree_dump = vcs_helpers.compress(vcs_helpers.dump_tree_fbs(tree))
    tree_hash = vcs_helpers.git_like_hash(tree_dump).digest()

    rev = vcs_helpers.SvnCommit(0, '', '', '', tree_hash, parent=None)
    rev_dump = vcs_helpers.compress(vcs_helpers.dump_svn_commit_fbs(rev))
    rev_hash = vcs_helpers.git_like_hash('0').digest()

    with open(svn_head_file, 'w') as f:
        f.write('0')

    vcs_store.put(tree_hash, consts.NODE_TREE, tree_dump)
    vcs_store.put(rev_hash, consts.NODE_SVN_CI, rev_dump)


def navigate_entry(vcs_store, root_object, path):
    parts = path.lstrip('/').split('/')

    cur = root_object
    for part in parts[:-1]:
        for e in cur.entries:
            if e.name == part:
                assert e.mode == consts.MODE_DIR
                stored_mode, cur_dump = vcs_store.get(e.hash)
                assert stored_mode == consts.NODE_TREE
                cur = vcs_helpers.load_tree_fbs(vcs_helpers.decompress(cur_dump))
                break

    for e in cur.entries:
        if e.name == parts[-1]:
            return e

    return None


def put_tree(vcs_store, entries):
    tree = vcs_helpers.Tree(sorted(entries, key=lambda e: (e.mode != consts.MODE_DIR, e.name)))
    tree_dump_fbs = vcs_helpers.dump_tree_fbs(tree)
    tree_dump = vcs_helpers.compress(tree_dump_fbs)
    tree_hash = vcs_helpers.git_like_hash(tree_dump).digest()
    vcs_store.put(tree_hash, consts.NODE_TREE, tree_dump)
    return tree, tree_hash, len(tree_dump_fbs)


class Wtf(object):
    REVISION = None
    DISABLED = False

    @classmethod
    def wtf(cls, msg):
        if cls.REVISION:
            s = 'While converting {} -> {}: '.format(cls.REVISION, cls.REVISION + 1)
        else:
            s = ''

        s += msg

        if cls.DISABLED:
            logging.warning(s)

        else:
            with open('wtf.txt', 'a') as f:
                f.write(s)
                f.write('\n')

    @classmethod
    def disable(cls):
        cls.DISABLED = True


def add_file(revision_root, path, store):
    props = revision_root.proplist(path)
    data_stream = revision_root.file_content(path)

    symlink = props.get('svn:special', None) == '*'
    executable = props.get('svn:executable', None) == '*'

    if symlink:
        preamble = data_stream.read(5)
        if preamble != 'link ':
            Wtf.wtf('not a link svn:special {}'.format(path))
            symlink = False
            data_stream = revision_root.file_content(path)

    if symlink and executable:
        Wtf.wtf('both svn:symlink and svn:executable {}'.format(path))
        executable = False

    assert not (symlink and executable)

    mode = consts.MODE_REG
    if symlink:
        mode = consts.MODE_LINK
    elif executable:
        mode = consts.MODE_EXEC

    blobs = []
    sha = hashlib.sha1()
    size = 0

    while True:
        data = data_stream.read(consts.YT_CHUNK - 4 * 1024)

        if blobs and not data:
            break

        compressed_data = vcs_helpers.compress(data)
        assert len(compressed_data) <= consts.YT_CHUNK
        compressed_data_hash = vcs_helpers.git_like_hash(compressed_data).digest()
        store.put(compressed_data_hash, consts.NODE_BLOB, compressed_data)
        blobs.append(compressed_data_hash)
        sha.update(data)
        size += len(data)

        if not data:
            assert blobs
            break

    sha.update('\0' + str(size))
    sha = sha.digest()

    return vcs_helpers.TreeEntry(path.split('/')[-1], mode, size, sha, blobs)


def add_dir(revision_root, path, store):
    entries = []

    for basename, kind in revision_root.dir_entries(path).iteritems():
        assert kind in (repos.FS_DIRENT_NODE_KIND_DIR, repos.FS_DIRENT_NODE_KIND_FILE, repos.FS_DIRENT_NODE_KIND_SYMLINK)

        if kind == repos.FS_DIRENT_NODE_KIND_DIR:
            entries.append(add_dir(revision_root, os.path.join(path, basename), store))

        elif kind in (repos.FS_DIRENT_NODE_KIND_FILE, repos.FS_DIRENT_NODE_KIND_SYMLINK):
            entries.append(add_file(revision_root, os.path.join(path, basename), store))

        else:
            raise Exception(kind)

    tree_object, tree_object_hash, tree_object_size = put_tree(store, entries)
    return vcs_helpers.TreeEntry(os.path.basename(path), consts.MODE_DIR, tree_object_size, tree_object_hash, [])


def add_entry(revision_root, path, store):
    if revision_root.is_file(path):
        return add_file(revision_root, path, store)
    else:
        return add_dir(revision_root, path, store)


def get_revision_and_root_objects(vcs_store, rev):
    rev_type, rev_obj_dump = vcs_store.get(vcs_helpers.git_like_hash(str(rev)).digest())
    assert rev_type == consts.NODE_SVN_CI
    rev_obj = vcs_helpers.load_svn_commit_fbs(vcs_helpers.decompress(rev_obj_dump))

    tree_type, tree_obj_dump = vcs_store.get(rev_obj.tree)
    assert tree_type == consts.NODE_TREE
    tree_obj = vcs_helpers.load_tree_fbs(vcs_helpers.decompress(tree_obj_dump))

    return rev_obj, tree_obj, len(tree_obj_dump)


def update(local_repo_fs, local_store, head, blacklist):
    Wtf.REVISION = head

    head_commit, head_tree, head_tree_size, = get_revision_and_root_objects(local_store, head)

    new_commit_root = local_repo_fs.revision_root(head + 1)
    new_commit_meta = local_repo_fs.revision_proplist(head + 1)
    new_commit_paths_changed = new_commit_root.paths_changed(copy_info=True)

    changed_path_to_entry = {}

    def has_blcklisted_subtree(p):
        p = p.rstrip('/') + '/'
        return any(b.startswith(p) for b in blacklist)  # blacklist paths must end with '/'

    def in_blacklisted_subtree(p):
        p = p.rstrip('/') + '/'
        return any(p.startswith(b) for b in blacklist)  # blacklist paths must end with '/'

    tree = paths_tree.PathsTree()
    added_paths = []

    for path, (node_rev_id, change_kind, text_mod, prop_mod, src_path, src_rev) in new_commit_paths_changed.iteritems():
        if not in_blacklisted_subtree(path):
            if text_mod:
                changed_path_to_entry[path] = add_file(new_commit_root, path, local_store)

            if src_rev != -1 or src_path is not None:
                assert src_rev != -1 and src_path is not None

                if in_blacklisted_subtree(src_path):
                    Wtf.wtf('secure move {}@{} -> {}@{}'.format(src_path, src_rev, path, head + 1))
                    changed_path_to_entry[path] = add_entry(new_commit_root, path, local_store)

            if prop_mod and new_commit_root.is_file(path):
                changed_path_to_entry[path] = add_file(new_commit_root, path, local_store)  # TODO

            tree.add_path(path.lstrip('/'), (node_rev_id, change_kind, text_mod, prop_mod, src_path, src_rev))
            added_paths.append(path.lstrip('/'))

    logging.info('Convert %s -> %s (%s)', str(head), str(head + 1), ' '.join(added_paths))

    def visit(path, node, base_tree):
        name_to_entry = {}

        if base_tree is not None:
            for entry in base_tree.entries:
                name_to_entry[entry.name] = entry

        for child_name, child_node in node['children'].iteritems():
            child_path = os.path.join(path, child_name)

            if 'value' not in child_node:
                assert base_tree is not None
                child_tree = vcs_helpers.load_tree_fbs(
                    vcs_helpers.decompress(local_store.get(navigate_entry(local_store, base_tree, child_name).hash)[1])
                )
                assert child_tree is not None
                name_to_entry[child_name] = visit(child_path, child_node, child_tree)
                continue

            else:
                node_rev_id, change_kind, text_mod, prop_mod, src_path, src_rev = child_node['value']

                if change_kind == repos.PATH_CHANGE_DELETE:
                    assert src_rev == -1 and src_path is None
                    name_to_entry[child_name] = None
                    continue

                if child_path in changed_path_to_entry:
                    name_to_entry[child_name] = changed_path_to_entry[child_path]
                    continue

                assert not text_mod

                if change_kind == repos.PATH_CHANGE_MODIFY:
                    # Changed directory svn-property - ignore
                    assert prop_mod
                    assert base_tree is not None

                    child_tree = vcs_helpers.load_tree_fbs(
                        vcs_helpers.decompress(
                            local_store.get(navigate_entry(local_store, base_tree, child_name).hash)[1]
                        )
                    )
                    assert child_tree is not None

                    name_to_entry[child_name] = visit(child_path, child_node, child_tree)
                    continue

                if src_rev != -1 or src_path is not None:
                    assert src_rev != -1 and src_path is not None
                    src_commit_object, src_tree_object, _ = get_revision_and_root_objects(local_store, src_rev)
                    if src_path == '/':
                        src_entry = vcs_helpers.TreeEntry('', consts.MODE_DIR, head_tree_size, head_commit.tree, [])
                    else:
                        src_entry = navigate_entry(local_store, src_tree_object, src_path)
                    assert src_entry is not None, src_path + '@' + str(src_rev)

                    if has_blcklisted_subtree(src_path):
                        Wtf.wtf('secure move {}@{} -> {}@{}'.format(src_path, src_rev, path, head + 1))

                    if src_entry.mode != consts.MODE_DIR or not child_node['children']:
                        src_entry.name = child_name
                        name_to_entry[child_name] = src_entry
                        continue

                    else:
                        child_base_tree = vcs_helpers.load_tree_fbs(
                            vcs_helpers.decompress(local_store.get(src_entry.hash)[1])
                        )
                        name_to_entry[child_name] = visit(child_path, child_node, child_base_tree)
                        continue

                name_to_entry[child_name] = visit(child_path, child_node, None)
                continue

        entries = [e for e in name_to_entry.values() if e is not None]
        tree_object, tree_object_hash, tree_object_size = put_tree(local_store, entries)

        return vcs_helpers.TreeEntry(os.path.basename(path), consts.MODE_DIR, tree_object_size, tree_object_hash, [])

    if added_paths:
        new_root_entry = visit('/', tree.root(), head_tree)
    else:
        new_root_entry = vcs_helpers.TreeEntry('/', consts.MODE_DIR, 0, head_commit.tree, [])

    new_root_hash = new_root_entry.hash
    new_commit = vcs_helpers.SvnCommit(
        head + 1,
        new_commit_meta.get('svn:author', ''),
        new_commit_meta.get('svn:date', ''),
        new_commit_meta.get('svn:log', ''),
        new_root_hash,
        vcs_helpers.git_like_hash(str(head)).digest()
    )

    local_store.put(
        vcs_helpers.git_like_hash(str(head + 1)).digest(),
        consts.NODE_SVN_CI,
        vcs_helpers.compress(vcs_helpers.dump_svn_commit_fbs(new_commit))
    )

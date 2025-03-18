import os
import json
import time
import logging
import hashlib

import vcs_helpers
import aapi.lib.py_common.consts as consts
import aapi.lib.py_common.paths_tree as paths_tree


# File state
MOD = 1
ADD = 2
DEL = 3


def put_blobs(content, store):
    blobs = []
    sha = hashlib.sha1()
    size = 0

    while True:
        data = content[size: size + consts.YT_CHUNK - 4 * 1024]
        size += len(data)

        if blobs and not data:
            break

        compressed_data = vcs_helpers.compress(data)
        assert len(compressed_data) <= consts.YT_CHUNK
        compressed_data_hash = vcs_helpers.git_like_hash(compressed_data).digest()
        store.put(compressed_data_hash, consts.NODE_BLOB, compressed_data)
        blobs.append(compressed_data_hash)
        sha.update(data)

        if not data:
            assert blobs
            break

    sha.update('\0' + str(size))

    return size, sha.digest(), blobs


def put_file(path, hash_, flags, ctx, store, log_str):
    logging.info(log_str, path)
    size, hash, blobs = put_blobs(ctx.data(), store)

    symlink = 'l' in flags
    executable = 'x' in flags and not symlink

    mode = consts.MODE_REG
    if symlink:
        mode = consts.MODE_LINK
    elif executable:
        mode = consts.MODE_EXEC

    return vcs_helpers.TreeEntry(os.path.basename(path), mode, size, hash, blobs)


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


def fix_s(s):
    if isinstance(s, unicode):
        return s.encode('utf-8')

    try:
        return s.decode('utf-8').encode('utf-8')
    except Exception:
        if len(s) == 20:
            # Maybe its a hash?
            try:
                return s.decode('hex')
            except Exception:
                return s.decode('utf-8', errors='replace').encode('utf-8')


def dump_extra_robust(extra):
    try:
        return json.dumps(extra)
    except UnicodeDecodeError:
        fixed_extra = {}

        for k, v in extra.iteritems():
            fixed_extra[fix_s(k)] = fix_s(v)

    return json.dumps(fixed_extra)


def convert_delta(pool, repo, store, base_tree_hash, delta, state, log_str):
    logging.info(log_str, ', '.join(sorted(delta.keys())))
    log_str = log_str % '{} of {} %s'
    tree = paths_tree.PathsTree()
    file_to_entry = {}

    for i, p in enumerate(delta):
        (h1, f1), (h2, f2) = delta[p]

        if h1 is None:
            assert h2 is not None
            tree.add_path(p, ADD)
            file_to_entry[p] = pool.submit(state.wrap_stopping, put_file, p, h2, f2, repo.filectx(p, fileid=h2), store, log_str.format(i, len(delta)))
            continue

        if h2 is None:
            assert h1 is not None
            tree.add_path(p, DEL)
            continue

        tree.add_path(p, MOD)
        file_to_entry[p] = pool.submit(state.wrap_stopping, put_file, p, h2, f2, repo.filectx(p, fileid=h2), store, log_str.format(i, len(delta)))

    if base_tree_hash is None:
        prev_tree = vcs_helpers.Tree([])
    else:
        prev_tree = vcs_helpers.load_tree_fbs(vcs_helpers.decompress(store.get(base_tree_hash, try_walk=True)[1]))

    def visit(path, node, tree_node):
        name_to_entry = {}

        if tree_node is not None:
            for entry in tree_node.entries:
                name_to_entry[entry.name] = entry

        for child_name, child_node in node['children'].iteritems():
            child_path = os.path.join(path, child_name)

            if child_path in file_to_entry:
                name_to_entry[child_name] = file_to_entry.pop(child_path).result()
                continue

            if child_node['children']:
                child_tree_node = None

                if tree_node is not None:
                    ent = navigate_entry(store, tree_node, child_name)

                    if ent is not None and ent.mode == consts.MODE_DIR:
                        child_tree_node = vcs_helpers.load_tree_fbs(vcs_helpers.decompress(store.get(ent.hash, try_walk=True)[1]))

                name_to_entry[child_name] = visit(child_path, child_node, child_tree_node)
                continue

            assert 'value' in child_node and child_node['value'] == DEL
            name_to_entry[child_name] = None

        entries = [e for e in name_to_entry.values() if e is not None]
        tree_object, tree_object_hash, tree_object_size = put_tree(store, entries)

        if entries:
            return vcs_helpers.TreeEntry(os.path.basename(path), consts.MODE_DIR, tree_object_size, tree_object_hash, [])
        else:
            return None

    new_root_ent = visit('', tree.root(), prev_tree)
    assert not file_to_entry, 'changed files are not used'

    if new_root_ent is None:
        # Arcadia deleted
        _, empty_tree_hash, _ = put_tree(store, [])
        return empty_tree_hash

    else:
        return new_root_ent.hash


def add_changeset(repo, store, new_tree_hash, rev):
    ctx = repo[rev]

    cs = vcs_helpers.HgChangeset(
        hash=ctx.node(),
        author=ctx.user(),
        date=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(ctx.date()[0])),
        msg=ctx.description(),
        files=[],
        branch=ctx.branch(),
        close_branch=ctx.closesbranch(),
        extra=dump_extra_robust(ctx.extra()),
        tree=new_tree_hash,
        parents=[p.node() for p in ctx.parents()]
    )

    assert len(cs.hash) == 20
    assert all(len(p) == 20 for p in cs.parents)

    store.put(ctx.node(), consts.NODE_HG_CHANGESET, vcs_helpers.compress(vcs_helpers.dump_hg_changeset_fbs(cs)))

    return ctx.node()

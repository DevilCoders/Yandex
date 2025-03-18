import os
import pytest
import stat
import library.python.zipatch as zipatch
import yatest.common

from . import common


def test_store_file(wc, store_patch):
    store_patch, affected_paths = store_patch[0], common.affected_paths(store_patch[1], wc)
    z = zipatch.Zipatch(store_patch)
    z.apply(wc)

    existing_file = os.path.join(wc, 'dir1', 'file_to_modify.txt')
    new_file = os.path.join(wc, 'dir1', 'file_added.txt')
    new_dir_file = os.path.join(wc, 'test_store_file', 'nested_new_dir', 'file_in_new_dir.txt')

    assert os.path.exists(existing_file)
    assert os.path.exists(new_file)
    assert os.path.exists(new_dir_file)

    executable_mode = stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
    for a in z.actions:
        file_exec_mode = os.stat(os.path.join(wc, a['path'])).st_mode & executable_mode
        if a.get('executable', False):
            assert file_exec_mode == executable_mode
        else:
            assert file_exec_mode == 0

    common.check_rigths(wc, affected_paths)
    return [yatest.common.canonical_file(existing_file, local=True), yatest.common.canonical_file(new_file, local=True), yatest.common.canonical_file(new_dir_file, local=True)]


def test_remove_file(wc, remove_patch):
    remove_patch, affected_paths = remove_patch[0], common.affected_paths(remove_patch[1], wc)
    z = zipatch.Zipatch(remove_patch)
    z.apply(wc)

    deleted_file = os.path.join(wc, 'dir1', 'file_to_remove.txt')

    common.check_rigths(wc, affected_paths)

    assert not os.path.exists(deleted_file)


def test_remove_tree(wc, remove_tree_patch):
    remove_tree_patch, affected_paths = remove_tree_patch[0], common.affected_paths(remove_tree_patch[1], wc)
    z = zipatch.Zipatch(remove_tree_patch)
    z.apply(wc)

    deleted_dir = os.path.join(wc, 'dir2')

    common.check_rigths(wc, affected_paths)
    assert not os.path.exists(deleted_dir)


def test_copy_file(wc, copy_patch):
    copy_patch, affected_paths = copy_patch[0], common.affected_paths(copy_patch[1], wc)
    z = zipatch.Zipatch(copy_patch)
    z.apply(wc)

    copied_unmodified = os.path.join(wc, 'test_copy_file', 'copied_unmodified.txt')
    copied_unmodified_at = os.path.join(wc, 'test_copy_file', 'copied_unmodified@.txt')
    copied_unmodified_curly = os.path.join(wc, 'test_copy_file', 'copied_unmodified{.txt')
    copied_modified = os.path.join(wc, 'test_copy_file', 'copied_modified.txt')
    copied_from_rev = os.path.join(wc, 'test_copy_file', 'copied_from_rev.txt')

    assert os.path.exists(copied_unmodified)
    assert os.path.exists(copied_unmodified_at)
    assert os.path.exists(copied_unmodified_curly)
    assert os.path.exists(copied_modified)
    assert os.path.exists(copied_from_rev)

    common.check_rigths(wc, affected_paths)

    return [
        yatest.common.canonical_file(copied_unmodified, local=True),
        yatest.common.canonical_file(copied_unmodified_at, local=True),
        yatest.common.canonical_file(copied_unmodified_curly, local=True),
        yatest.common.canonical_file(copied_modified, local=True),
        yatest.common.canonical_file(copied_from_rev, local=True)
    ]


def test_mkdir(wc, mkdir_patch):
    mkdir_patch, affected_paths = mkdir_patch[0], common.affected_paths(mkdir_patch[1], wc)
    z = zipatch.Zipatch(mkdir_patch)
    z.apply(wc)

    for zipatch_dir in z.paths:
        abs_dir = os.path.join(wc, zipatch_dir)

        assert os.path.exists(abs_dir)
        assert os.path.isdir(abs_dir)

    common.check_rigths(wc, affected_paths)


def test_store_file_ro(wc, store_patch):
    store_patch, affected_paths = store_patch[0], common.affected_paths(store_patch[1], wc)
    z = zipatch.Zipatch(store_patch)
    z.apply(wc, write_access=False)
    common.check_rigths(wc, affected_paths, write_access=False)


def test_remove_file_ro(wc, remove_patch):
    remove_patch, affected_paths = remove_patch[0], common.affected_paths(remove_patch[1], wc)
    z = zipatch.Zipatch(remove_patch)
    z.apply(wc, write_access=False)
    common.check_rigths(wc, affected_paths, write_access=False)


def test_remove_tree_ro(wc, remove_tree_patch):
    remove_tree_patch, affected_paths = remove_tree_patch[0], common.affected_paths(remove_tree_patch[1], wc)
    z = zipatch.Zipatch(remove_tree_patch)
    z.apply(wc, write_access=False)
    common.check_rigths(wc, affected_paths, write_access=False)


def test_copy_file_ro(wc, copy_patch):
    copy_patch, affected_paths = copy_patch[0], common.affected_paths(copy_patch[1], wc)
    z = zipatch.Zipatch(copy_patch)
    z.apply(wc, write_access=False)
    common.check_rigths(wc, affected_paths, write_access=False)


def test_mkdir_ro(wc, mkdir_patch):
    mkdir_patch, affected_paths = mkdir_patch[0], common.affected_paths(mkdir_patch[1], wc)
    z = zipatch.Zipatch(mkdir_patch)
    z.apply(wc, write_access=False)
    common.check_rigths(wc, affected_paths, write_access=False)


def test_broken(wc, broken_patch):
    z = zipatch.Zipatch(broken_patch[0])
    with pytest.raises(zipatch.misc.ZipatchApplyError) as e:
        z.apply(wc, write_access=False)
    assert 'Failed to chmod' in e.value.args[0]

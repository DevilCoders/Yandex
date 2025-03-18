import os
import tempfile
import stat

import yatest.common
import subvertpy.repos
import subvertpy.wc

import library.python.func as func
import library.python.windows as win
import library.python.zipatch as zipatch
import vcs.svn.wc.client

from .conftest import path_to_url

from . import common


# see https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/subversion/subversion/include/svn_wc.h?rev=3873892#L3426
SVN_WC_ENTRY_THIS_DIR = ''


class WorkingCopyEntry(object):
    def __init__(self, wc_status_entry, path, wco):
        for attr in ("entry", "locked", "copied", "switched", "url", "revision", "kind", "status"):
            setattr(self, attr, getattr(wc_status_entry, attr))
        self._wco = wco
        self._path = path

    @property
    def copyfrom_url(self):
        return self.entry.copyfrom_url

    @property
    def copyfrom_rev(self):
        return self.entry.copyfrom_rev

    @property
    def copyfrom_path(self):
        return None if self.entry.copyfrom_url is None else self.entry.copyfrom_url.replace(self.entry.repos, '')

    @property
    def is_copy_root(self):
        return bool(self.entry.copyfrom_url and self.entry.copyfrom_rev)

    @property
    def is_dir(self):
        return self.entry.kind == subvertpy.repos.FS_DIRENT_NODE_KIND_DIR

    @property
    def is_file(self):
        return self.entry.kind == subvertpy.repos.FS_DIRENT_NODE_KIND_FILE

    @property
    def added(self):
        return self.status == subvertpy.wc.STATUS_ADDED

    @property
    def removed(self):
        return self.status == subvertpy.wc.STATUS_DELETED

    @property
    def replaced(self):
        return self.status == subvertpy.wc.STATUS_REPLACED

    @func.lazy_property
    def text_modified(self):
        return self._wco.text_modified(self._path)

    @func.lazy_property
    def props_modified(self):
        return self._wco.props_modified(self._path)

    @property
    def normal(self):
        return self.status == subvertpy.wc.STATUS_NORMAL


def svn_st(path, wco=None):
    res = {}

    if wco is None:
        wco = subvertpy.wc.WorkingCopy(associated=None, path=path, write_lock=False, depth=-1)

    def st_callback(subpath, wc_entry):
        if wc_entry.name != SVN_WC_ENTRY_THIS_DIR:
            res[subpath] = WorkingCopyEntry(wco.status(subpath), subpath, wco)

    wco.walk_entries(path, st_callback, False)
    return res


def test_store_file(wc_info, repo_path, store_patch):
    wc = wc_info['path']
    executable_file = os.path.join(wc, 'dir3', 'file_to_unset_executable_bit.txt')
    not_executable_file = os.path.join(wc, 'dir3', 'file_to_set_executable_bit.txt')
    encrypted_file = os.path.join(wc, 'dir3', 'file_to_unset_encrypted_bit.txt')
    not_encrypted_file = os.path.join(wc, 'dir3', 'file_to_set_encrypted_bit.txt')

    def is_executable(path):
        return bool(os.stat(path).st_mode & (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH))

    if not win.on_win():
        assert is_executable(executable_file)
        assert not is_executable(not_executable_file)

    binary_file = os.path.join(wc, 'dir3', 'file_to_fail_unset_binary_hint.txt')
    with vcs.svn.wc.client.SvnClient() as svn:
        binary_file_mime = svn.propget('svn:mime-type', binary_file, None, None)

    binary_file_not_binary = os.path.join(wc, 'dir3', 'file_to_unset_binary_hint.txt')
    with vcs.svn.wc.client.SvnClient() as svn:
        assert svn.propget('svn:mime-type', binary_file_not_binary, None, None)[binary_file_not_binary] == b'application/octet-stream'

    store_patch, affected_paths = store_patch[0], common.affected_paths(store_patch[1], wc)
    z = zipatch.ZipatchSvn(store_patch)
    z.apply(wc)

    state = svn_st(wc)

    assert os.path.exists(binary_file)

    with vcs.svn.wc.client.SvnClient() as svn:
        assert binary_file_mime == svn.propget('svn:mime-type', binary_file, None, None)
        assert not svn.propget('svn:mime-type', binary_file_not_binary, None, None)

    # there should be no changes to /dir3/file_to_fail_unset_binary_hint.txt
    entry = state[binary_file]
    assert not entry.text_modified and not entry.props_modified and entry.normal

    assert os.path.exists(executable_file)
    assert os.path.exists(not_executable_file)

    if not win.on_win():
        assert not is_executable(executable_file)
        assert is_executable(not_executable_file)
    with vcs.svn.wc.client.SvnClient() as svn:
        assert not svn.propget('svn:executable', executable_file, None, None)
        assert svn.propget('svn:executable', not_executable_file, None, None)

    entry = state[executable_file]
    assert entry.text_modified and entry.props_modified

    entry = state[not_executable_file]
    assert entry.text_modified and entry.props_modified

    assert os.path.exists(encrypted_file)
    assert os.path.exists(not_encrypted_file)

    with vcs.svn.wc.client.SvnClient() as svn:
        assert not svn.propget('arc:encrypted', encrypted_file, None, None)
        assert svn.propget('arc:encrypted', not_encrypted_file, None, None) == {not_encrypted_file: b'*'}

    entry = state[encrypted_file]
    assert entry.text_modified and entry.props_modified

    entry = state[not_encrypted_file]
    assert entry.text_modified and entry.props_modified

    existing_file = os.path.join(wc, 'dir1', 'file_to_modify.txt')
    new_file = os.path.join(wc, 'dir1', 'file_added.txt')
    new_dir_file = os.path.join(wc, 'test_store_file', 'nested_new_dir', 'file_in_new_dir.txt')

    assert os.path.exists(existing_file)
    assert state[existing_file].text_modified

    assert os.path.exists(new_file)
    assert state[new_file].added

    assert os.path.exists(new_dir_file)

    entry = state[os.path.join(wc, 'test_store_file')]
    assert entry.is_dir and entry.added

    entry = state[os.path.join(wc, 'test_store_file', 'nested_new_dir')]
    assert entry.is_dir and entry.added

    entry = state[new_dir_file]
    assert entry.is_file and entry.added

    common.check_rigths(wc, affected_paths)

    return [yatest.common.canonical_file(existing_file, local=True), yatest.common.canonical_file(new_file, local=True), yatest.common.canonical_file(new_dir_file, local=True)]


def test_remove_file(wc_info, repo_path, remove_patch):
    wc = wc_info['path']
    remove_patch, affected_paths = remove_patch[0], common.affected_paths(remove_patch[1], wc)
    z = zipatch.ZipatchSvn(remove_patch)
    z.apply(wc)

    deleted_file = os.path.join(wc, 'dir1', 'file_to_remove.txt')

    assert not os.path.exists(deleted_file)

    state = svn_st(wc)

    assert state[deleted_file].removed

    common.check_rigths(wc, affected_paths)


def test_remove_tree(wc_info, repo_path, remove_tree_patch):
    wc = wc_info['path']
    remove_tree_patch, affected_paths = remove_tree_patch[0], common.affected_paths(remove_tree_patch[1], wc)
    z = zipatch.ZipatchSvn(remove_tree_patch)
    z.apply(wc)

    deleted_dir = os.path.join(wc, 'dir2')

    assert not os.path.exists(deleted_dir)

    state = svn_st(wc)

    assert state[os.path.join(wc, 'dir2')].removed

    common.check_rigths(wc, affected_paths)


def test_copy_file(wc_info, repo_path, copy_patch):
    wc = wc_info['path']
    copy_patch, affected_paths = copy_patch[0], common.affected_paths(copy_patch[1], wc)
    z = zipatch.ZipatchSvn(copy_patch)
    z.apply(wc)

    dir = os.path.join(wc, 'test_copy_file')
    copied_unmodified = os.path.join(dir, 'copied_unmodified.txt')
    copied_unmodified_at = os.path.join(dir, 'copied_unmodified@.txt')
    copied_unmodified_curly = os.path.join(dir, 'copied_unmodified{.txt')
    copied_modified = os.path.join(dir, 'copied_modified.txt')
    copied_from_rev = os.path.join(dir, 'copied_from_rev.txt')
    copied_from_br = os.path.join(dir, 'copied_from_branch_unmodified.txt')
    copied_from_br_modified = os.path.join(dir, 'copied_from_branch_modified.txt')

    state = svn_st(wc)
    base_rev = int(wc_info['rev'])

    assert state[dir].added and not state[dir].copied

    assert state[copied_unmodified].copied and not state[copied_unmodified].text_modified
    assert state[copied_unmodified].copyfrom_path == '/trunk/dir1/file_to_copy.txt'
    assert state[copied_unmodified].copyfrom_rev == base_rev

    assert state[copied_unmodified_at].copied and not state[copied_unmodified_at].text_modified
    assert state[copied_unmodified_at].copyfrom_path == '/trunk/dir1/file_to_copy@.txt'
    assert state[copied_unmodified_at].copyfrom_rev == base_rev

    assert state[copied_unmodified_curly].copied and not state[copied_unmodified_curly].text_modified
    assert state[copied_unmodified_curly].copyfrom_path == '/trunk/dir1/file_to_copy%7B.txt'
    assert state[copied_unmodified_curly].copyfrom_rev == base_rev

    assert state[copied_modified].copied and state[copied_modified].text_modified
    assert state[copied_modified].copyfrom_path == '/trunk/dir1/file_to_copy.txt'
    assert state[copied_modified].copyfrom_rev == base_rev

    assert state[copied_from_rev].copied and not state[copied_from_rev].text_modified
    assert state[copied_from_rev].copyfrom_path == '/trunk/dir1/file_to_copy.txt'
    assert state[copied_from_rev].copyfrom_rev == 2

    assert state[copied_from_br].copied and not state[copied_from_br].text_modified
    assert state[copied_from_br].copyfrom_path == '/branches/br1/dir1/file_to_copy.txt'
    assert state[copied_from_br].copyfrom_rev == base_rev

    assert state[copied_from_br_modified].copied and state[copied_from_br_modified].text_modified
    assert state[copied_from_br_modified].copyfrom_path == '/branches/br1/dir1/file_to_modify.txt'
    assert state[copied_from_br_modified].copyfrom_rev == base_rev

    common.check_rigths(wc, affected_paths)

    return yatest.common.canonical_dir(dir)


def test_copy_in_branch(repo_path, copy_patch):
    z = zipatch.ZipatchSvn(copy_patch[0])

    base_revision = 4
    repo_url = path_to_url(os.path.join(repo_path, 'branches', 'br1'))
    wc = os.path.join(tempfile.mkdtemp(), 'test_wc')
    with vcs.svn.wc.client.SvnClient() as svn:
        svn.checkout(repo_url, wc, base_revision, recurse=True)
    assert subvertpy.wc.check_wc(wc)

    with vcs.svn.wc.client.SvnClient() as svn:
        head_rev = svn.info(repo_url).popitem()[1].revision
    affected_paths = common.affected_paths(copy_patch[1], wc)
    z.apply(wc)

    state = svn_st(wc)
    dir = os.path.join(wc, 'test_copy_file')
    copied_unmodified = os.path.join(dir, 'copied_unmodified.txt')
    copied_unmodified_at = os.path.join(dir, 'copied_unmodified@.txt')
    copied_unmodified_curly = os.path.join(dir, 'copied_unmodified{.txt')
    copied_modified = os.path.join(dir, 'copied_modified.txt')
    copied_from_rev = os.path.join(dir, 'copied_from_rev.txt')
    copied_from_br = os.path.join(dir, 'copied_from_branch_unmodified.txt')
    copied_from_br_modified = os.path.join(dir, 'copied_from_branch_modified.txt')

    assert state[dir].added and not state[dir].copied

    assert state[copied_unmodified].copied and not state[copied_unmodified].text_modified
    assert state[copied_unmodified].copyfrom_path == '/trunk/dir1/file_to_copy.txt'
    assert state[copied_unmodified].copyfrom_rev == head_rev

    assert state[copied_unmodified_at].copied and not state[copied_unmodified_at].text_modified
    assert state[copied_unmodified_at].copyfrom_path == '/trunk/dir1/file_to_copy@.txt'
    assert state[copied_unmodified_at].copyfrom_rev == head_rev

    assert state[copied_unmodified_curly].copied and not state[copied_unmodified_curly].text_modified
    assert state[copied_unmodified_curly].copyfrom_path == '/trunk/dir1/file_to_copy%7B.txt'
    assert state[copied_unmodified_curly].copyfrom_rev == head_rev

    assert state[copied_modified].copied and state[copied_modified].text_modified
    assert state[copied_modified].copyfrom_path == '/trunk/dir1/file_to_copy.txt'
    assert state[copied_modified].copyfrom_rev == head_rev

    assert state[copied_from_rev].copied and not state[copied_from_rev].text_modified
    assert state[copied_from_rev].copyfrom_path == '/trunk/dir1/file_to_copy.txt'
    assert state[copied_from_rev].copyfrom_rev == 2

    assert state[copied_from_br].copied and not state[copied_from_br].text_modified
    assert state[copied_from_br].copyfrom_path == '/branches/br1/dir1/file_to_copy.txt'
    assert state[copied_from_br].copyfrom_rev == head_rev

    assert state[copied_from_br_modified].copied and state[copied_from_br_modified].text_modified
    assert state[copied_from_br_modified].copyfrom_path == '/branches/br1/dir1/file_to_modify.txt'
    assert state[copied_from_br_modified].copyfrom_rev == head_rev

    common.check_rigths(wc, affected_paths)

    return yatest.common.canonical_dir(dir)


def test_nested_copy(wc_info, repo_path, nested_copy_patch):
    wc = wc_info['path']
    nested_copy_patch, affected_paths = nested_copy_patch[0], common.affected_paths(nested_copy_patch[1], wc)
    z = zipatch.ZipatchSvn(nested_copy_patch)
    z.apply(wc)

    state = svn_st(wc)
    dir_unmodified = os.path.join(wc, 'test_nested_copy')
    copied1 = os.path.join(dir_unmodified, 'file_to_modify.txt')
    copied2 = os.path.join(dir_unmodified, 'file_to_copy.txt')
    copied3 = os.path.join(dir_unmodified, 'file_to_remove.txt')
    dir_nested = os.path.join(dir_unmodified, 'nested_dir1')
    copied_nested = os.path.join(dir_nested, 'file_to_replace_with.txt')

    assert state[dir_unmodified].copied and state[dir_unmodified].is_dir
    assert state[dir_unmodified].copyfrom_path == '/trunk/dir1'
    assert state[dir_unmodified].copyfrom_rev == 2

    assert state[copied1].copied and not state[copied1].is_copy_root and state[copied1].normal and not state[copied1].text_modified
    assert state[copied1].copyfrom_path is None and state[copied1].copyfrom_rev == -1

    assert state[copied2].copied and not state[copied2].is_copy_root and state[copied2].normal and not state[copied2].text_modified
    assert state[copied2].copyfrom_path is None and state[copied2].copyfrom_rev == -1

    assert state[copied3].copied and not state[copied3].is_copy_root and state[copied3].normal and not state[copied3].text_modified
    assert state[copied3].copyfrom_path is None and state[copied3].copyfrom_rev == -1

    assert state[dir_nested].copied and state[dir_nested].is_dir and state[dir_nested].replaced
    assert state[dir_nested].copyfrom_path == '/trunk/dir2'
    assert state[dir_nested].copyfrom_rev == 2

    assert not state[copied_nested].copied and state[copied_nested].added and state[copied_nested].text_modified
    assert state[copied_nested].copyfrom_path is None and state[copied_nested].copyfrom_rev == -1

    with open(os.path.join(wc, 'test_nested_copy', 'file_to_modify.txt'), 'r') as f:
        assert f.read() == 'this line was here\n'
    with open(os.path.join(wc, 'test_nested_copy', 'file_to_copy.txt'), 'r') as f:
        assert f.read() == 'first version\n'
    with open(os.path.join(wc, 'test_nested_copy', 'file_to_remove.txt'), 'r') as f:
        assert f.read() == 'this file would be removed\n'

    with open(os.path.join(wc, 'test_nested_copy', 'nested_dir1', 'file_to_replace_with.txt'), 'r') as f:
        assert f.read() == 'this file survived\n'

    common.check_rigths(wc, affected_paths)


# DEVTOOLS-4534
def test_copy_into_existent_dir(wc_info, repo_path):
    wc = wc_info['path']
    os.makedirs(os.path.join(wc, 'unversioned_dir'))

    patch = tempfile.NamedTemporaryFile(delete=False)
    z = zipatch.ZipatchWriter()
    z.add_action('svn_copy', 'unversioned_dir/target', orig_path='trunk/dir1')
    z.add_action('mkdir', 'unversioned_dir/target')
    z.add_action('store_file', 'unversioned_dir/target/file_to_copy.txt', data='modified data')
    z.save(patch.name)
    patch.close()

    z = zipatch.ZipatchSvn(patch.name)
    z.apply(wc)

    assert os.path.isdir(os.path.join(wc, 'unversioned_dir'))
    assert os.path.exists(os.path.join(wc, 'unversioned_dir/target'))
    assert os.path.exists(os.path.join(wc, 'unversioned_dir/target/file_to_copy.txt'))
    os.remove(patch.name)


def test_mkdir(wc, mkdir_patch, repo_path):
    mkdir_patch, affected_paths = mkdir_patch[0], common.affected_paths(mkdir_patch[1], wc)
    z = zipatch.ZipatchSvn(mkdir_patch)
    z.apply(wc)

    state = svn_st(wc)

    dir1 = os.path.join(wc, 'zipatch_new_dir')
    dir2 = os.path.join(wc, 'zipatch_new_dir2')
    dir3 = os.path.join(dir2, 'inside')
    dir4 = os.path.join(wc, 'test_svn_copy_and_mkdir')

    for dir in [dir1, dir2, dir3, dir4]:
        assert state[dir].is_dir and state[dir].added

    assert state[dir4].copyfrom_path == '/trunk/dir2' and state[dir4].copyfrom_rev == 2

    common.check_rigths(wc, affected_paths)


def test_store_file_ro(wc, store_patch):
    store_patch, affected_paths = store_patch[0], common.affected_paths(store_patch[1], wc)
    z = zipatch.ZipatchSvn(store_patch)
    z.apply(wc, write_access=False)
    common.check_rigths(wc, affected_paths, write_access=True)

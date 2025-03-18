import os
import pytest
import shutil
import stat
import tempfile

import library.python.windows as win
import library.python.zipatch as zipatch
import subvertpy
import subvertpy.ra
import subvertpy.repos
import vcs.svn.wc.client


def to_strlist(s):
    return [bytes(s.encode('utf-8'))]


def path_to_url(s):
    # XXX DEVTOOLS-4498
    s = os.path.abspath(s)
    if win.on_win():
        s = '/{}'.format(s.replace(os.sep, '/'))
    return 'file://{}'.format(s)


@pytest.fixture(scope='module')
def base_revision():
    yield 3


@pytest.fixture(scope='module')
def repo_path():
    repo_dir = os.path.join(tempfile.mkdtemp(), 'repo')
    repo_url = path_to_url(repo_dir)
    with vcs.svn.wc.client.SvnClient() as svn:
        subvertpy.repos.create(repo_dir)

        wc_dir = tempfile.mkdtemp('init_wc')
        svn.checkout(repo_url, wc_dir, "HEAD")
        assert os.path.exists(wc_dir)
        assert subvertpy.wc.check_wc(wc_dir)

        # -------------------------- revision 1 ------------------------- #
        p = os.path.join(wc_dir, 'trunk')
        os.makedirs(p)
        svn.add(p)
        p = os.path.join(wc_dir, 'branches')
        os.makedirs(p)
        svn.add(p)
        subvertpy.client.log_msg_func = lambda s: 'init check-in'
        svn.commit(to_strlist(wc_dir))

        # -------------------------- revision 2 ------------------------- #
        p = os.path.join(wc_dir, 'trunk', 'dir1', 'file_to_modify.txt')
        os.makedirs(os.path.dirname(p))
        with open(p, 'wb') as f:
            f.write(b'''\
this line was here
''')

        p = os.path.join(wc_dir, 'trunk', 'dir1', 'file_to_copy.txt')
        with open(p, 'wb') as f:
            f.write(b'''\
first version
''')

        p = os.path.join(wc_dir, 'trunk', 'dir1', 'file_to_copy@.txt')
        with open(p, 'wb') as f:
            f.write(b'''\
first version @
''')

        p = os.path.join(wc_dir, 'trunk', 'dir1', 'file_to_copy{.txt')
        with open(p, 'wb') as f:
            f.write(b'''\
first version {
''')

        p = os.path.join(wc_dir, 'trunk', 'dir1', 'file_to_remove.txt')
        with open(p, 'wb') as f:
            f.write(b'''\
this file would be removed
''')
        p = os.path.join(wc_dir, 'trunk', 'dir1', 'nested_dir1')
        os.makedirs(p)
        p = os.path.join(wc_dir, 'trunk', 'dir1', 'nested_dir1', 'file_to_replace.txt')
        with open(p, 'wb') as f:
            f.write(b'this file would be replaced')

        svn.add(os.path.join(wc_dir, 'trunk', 'dir1'))

        p = os.path.join(wc_dir, 'trunk', 'dir2')
        os.makedirs(p)
        with open(os.path.join(p, 'file_to_remove.txt'), 'wb') as f:
            f.write(b'''\
#define GOOD_BYE_CRUEL_WORLD
''')
        os.makedirs(os.path.join(wc_dir, 'trunk', 'dir2', 'nested_dir_to_remove'))
        with open(os.path.join(wc_dir, 'trunk', 'dir2', 'file_to_replace_with'), 'wb') as f:
            f.write(b'this file would be copied and survive')
        svn.add(os.path.join(wc_dir, 'trunk', 'dir2'))

        os.makedirs(os.path.join(wc_dir, 'trunk', 'dir3'))
        p = os.path.join(wc_dir, 'trunk', 'dir3', 'file_to_unset_executable_bit.txt')
        with open(p, 'wb') as f:
            f.write(b'this file has executable attribute')
        os.chmod(p, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)

        p = os.path.join(wc_dir, 'trunk', 'dir3', 'file_to_set_executable_bit.txt')
        with open(p, 'wb') as f:
            f.write(b'this file has no executable attribute')

        p = os.path.join(wc_dir, 'trunk', 'dir3', 'file_to_unset_encrypted_bit.txt')
        with open(p, 'wb') as f:
            f.write(b'this file has encrypted attribute')

        p = os.path.join(wc_dir, 'trunk', 'dir3', 'file_to_set_encrypted_bit.txt')
        with open(p, 'wb') as f:
            f.write(b'this file has no encrypted attribute')

        p = os.path.join(wc_dir, 'trunk', 'dir3', 'file_to_set_binary_hint.txt')
        with open(p, 'wb') as f:
            f.write(b'this file has svn:mime-type == plain/text')

        svn.add(os.path.join(wc_dir, 'trunk', 'dir3'))
        if win.on_win():
            svn.propset('svn:executable', 'ON', os.path.join(wc_dir, 'trunk', 'dir3', 'file_to_unset_executable_bit.txt'))
        svn.propset('arc:encrypted', '*', os.path.join(wc_dir, 'trunk', 'dir3', 'file_to_unset_encrypted_bit.txt'))

        p = os.path.join(wc_dir, 'trunk', 'dir3', 'file_to_fail_unset_binary_hint.txt')
        with open(p, 'wb') as f:
            f.write(b'this file has svn:mime-type == application/octet-stream')
        svn.add(p)
        svn.propset('svn:mime-type', 'application/octet-stream', p)

        p = os.path.join(wc_dir, 'trunk', 'dir3', 'file_to_unset_binary_hint.txt')
        with open(p, 'wb') as f:
            f.write(b'this file has svn:mime-type == application/octet-stream')
        svn.add(p)
        svn.propset('svn:mime-type', 'application/octet-stream', p)

        subvertpy.client.log_msg_func = lambda s: 'init check-in for trunk'
        svn.commit(to_strlist(wc_dir))

        # -------------------------- revision 3 ------------------------- #
        p = os.path.join(wc_dir, 'trunk', 'dir1', 'file_to_copy.txt')
        with open(p, 'wb') as f:
            f.write(b'''\
second version
''')
        subvertpy.client.log_msg_func = lambda s: 'new version of file_to_copy.txt'
        svn.commit(to_strlist(wc_dir))

        # -------------------------- revision 4 ------------------------- #
        p = os.path.join(wc_dir, 'branches', 'br1')
        shutil.copytree(os.path.join(wc_dir, 'trunk'), p)
        svn.add(p)
        with open(os.path.join(p, 'dir1', 'file_to_modify.txt'), 'w') as f:
            f.write('''\
this line was here only in branch br1
''')
        subvertpy.client.log_msg_func = lambda s: 'new branch'
        svn.commit(to_strlist(wc_dir))

    yield repo_dir


@pytest.fixture(scope='function')  # XXX subvertpy has no svn revert yet, so checkout fresh new copy for each test
def wc_info(repo_path, base_revision):
    wc_dir = tempfile.mkdtemp('wc')
    repo_url = path_to_url(os.path.join(repo_path, 'trunk'))
    with vcs.svn.wc.client.SvnClient() as svn:
        rev = svn.checkout(repo_url, wc_dir, 'HEAD', recurse=True)
    assert subvertpy.wc.check_wc(wc_dir)
    return {'path': wc_dir, 'rev': rev}


@pytest.fixture(scope='function')
def wc(wc_info):
    return wc_info['path']


@pytest.fixture(scope='session')
def store_patch():
    patch = tempfile.NamedTemporaryFile(delete=False)

    z = zipatch.ZipatchWriter()

    z.add_action('store_file', 'dir1/file_to_modify.txt', data='''\
this line was here
and this is new one
''')
    z.add_action('store_file', 'dir1/file_added.txt', data='''\
file_added.txt
''')
    z.add_action('store_file', 'test_store_file/nested_new_dir/file_in_new_dir.txt', data='''\
file_in_new_dir
''')
    z.add_action('store_file', 'dir3/file_to_unset_executable_bit.txt', executable=False, data='''\
this file has executable attribute
''')
    z.add_action('store_file', 'dir3/file_to_set_executable_bit.txt', executable=True, data='''\
this file has no executable attribute
''')
    z.add_action('store_file', 'dir3/file_to_unset_encrypted_bit.txt', encrypted=False, data='''\
this file has encrypted attribute
''')
    z.add_action('store_file', 'dir3/file_to_set_encrypted_bit.txt', encrypted=True, data='''\
this file has no encrypted attribute
''')
    z.add_action('store_file', 'dir3/file_to_set_binary_hint.txt', binary_hint=True, data='''\
this file has svn:mime-type == plain/text
''')
    z.add_action('store_file', 'dir3/file_to_fail_unset_binary_hint.txt',
                 data='''this file has svn:mime-type == application/octet-stream''')
    z.add_action('store_file', 'dir3/file_to_unset_binary_hint.txt', binary_hint=False,
                 data='''this file has no svn:mime-type == application/octet-stream any more''')

    z.save(patch.name)
    yield patch.name, z.normpaths

    patch.close()
    os.remove(patch.name)


@pytest.fixture(scope='session')
def remove_patch():
    patch = tempfile.NamedTemporaryFile(delete=False)

    z = zipatch.ZipatchWriter()

    z.add_action('remove_file', 'dir1/file_to_remove.txt')
    z.save(patch.name)

    yield patch.name, z.normpaths

    patch.close()
    os.remove(patch.name)


@pytest.fixture(scope='session')
def broken_patch():
    patch = tempfile.NamedTemporaryFile(delete=False)

    z = zipatch.ZipatchWriter()

    z.add_action('store_file', 'dir1/file_to_modify.txt', data='''\
this line was here
and this is new one
''')
    z.add_action('remove_tree', 'dir1')
    z.save(patch.name)

    yield patch.name, z.normpaths

    patch.close()
    os.remove(patch.name)


@pytest.fixture(scope='session')
def remove_tree_patch():
    patch = tempfile.NamedTemporaryFile(delete=False)

    z = zipatch.ZipatchWriter()

    z.add_action('remove_tree', 'dir2')
    z.save(patch.name)

    yield patch.name, z.normpaths

    patch.close()
    os.remove(patch.name)


@pytest.fixture(scope='session')
def copy_patch():
    patch = tempfile.NamedTemporaryFile(delete=False)

    z = zipatch.ZipatchWriter()

    z.add_action('svn_copy', 'test_copy_file/copied_unmodified.txt', orig_path='trunk/dir1/file_to_copy.txt')
    z.add_action('store_file', 'test_copy_file/copied_unmodified.txt', data='''\
second version
''')
    z.add_action('svn_copy', 'test_copy_file/copied_unmodified@.txt', orig_path='trunk/dir1/file_to_copy@.txt')
    z.add_action('store_file', 'test_copy_file/copied_unmodified@.txt', data='''\
first version @
''')
    z.add_action('svn_copy', 'test_copy_file/copied_unmodified{.txt', orig_path='trunk/dir1/file_to_copy{.txt')
    z.add_action('store_file', 'test_copy_file/copied_unmodified{.txt', data='''\
first version {
''')
    z.add_action('svn_copy', 'test_copy_file/copied_modified.txt', orig_path='trunk/dir1/file_to_copy.txt')
    z.add_action('store_file', 'test_copy_file/copied_modified.txt', data='''\
second version
this line is added after svn copy
''')
    z.add_action('svn_copy', 'test_copy_file/copied_from_rev.txt', orig_path='trunk/dir1/file_to_copy.txt', orig_revision=2)
    z.add_action('store_file', 'test_copy_file/copied_from_rev.txt', data='''\
first version
''')
    z.add_action('svn_copy', 'test_copy_file/copied_from_branch_unmodified.txt', orig_path='branches/br1/dir1/file_to_copy.txt')
    z.add_action('store_file', 'test_copy_file/copied_from_branch_unmodified.txt', data='''\
second version
''')
    z.add_action('svn_copy', 'test_copy_file/copied_from_branch_modified.txt', orig_path='branches/br1/dir1/file_to_modify.txt')
    z.add_action('store_file', 'test_copy_file/copied_from_branch_modified.txt', data='''\
this is modified from branch br1
''')

    z.save(patch.name)

    yield patch.name, z.normpaths

    patch.close()
    os.remove(patch.name)


@pytest.fixture(scope='session')
def nested_copy_patch():
    patch = tempfile.NamedTemporaryFile(delete=False)

    z = zipatch.ZipatchWriter()

    # https://st.yandex-team.ru/ARCADIA-626
    z.add_action('svn_copy', 'test_nested_copy', orig_path='trunk/dir1', orig_revision=2)
    z.add_action('mkdir', 'test_nested_copy')
    z.add_action('store_file', 'test_nested_copy/file_to_modify.txt', data='''\
this line was here
''')
    z.add_action('store_file', 'test_nested_copy/file_to_copy.txt', data='''\
first version
''')
    z.add_action('store_file', 'test_nested_copy/file_to_remove.txt', data='''\
this file would be removed
''')
    z.add_action('svn_copy', 'test_nested_copy/nested_dir1', orig_path='trunk/dir2', orig_revision=2)
    z.add_action('mkdir', 'test_nested_copy/nested_dir1')
    z.add_action('store_file', 'test_nested_copy/nested_dir1/file_to_replace_with.txt', data='''\
this file survived
''')

    z.save(patch.name)

    yield patch.name, z.normpaths

    patch.close()
    os.remove(patch.name)


@pytest.fixture(scope='session')
def mkdir_patch():
    patch = tempfile.NamedTemporaryFile(delete=False)
    z = zipatch.ZipatchWriter()

    z.add_action('mkdir', 'zipatch_new_dir')
    z.add_action('mkdir', 'zipatch_new_dir2/inside')

    # zipatch created by real generator would also contain (at the end) store_file actions for every file in copied dir
    z.add_action('svn_copy', 'test_svn_copy_and_mkdir', orig_path='trunk/dir2', orig_revision=2)
    z.add_action('mkdir', 'test_svn_copy_and_mkdir')

    z.save(patch.name)

    yield patch.name, z.normpaths

    patch.close()
    os.remove(patch.name)

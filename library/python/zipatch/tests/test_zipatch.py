import json
import os
import pytest
import shutil
import stat
import tempfile
import zipfile

import library.python.windows as win
from library.python.zipatch import Zipatch, ZipatchWriter
from library.python.zipatch.validate import ZipatchMalformedError

from . import common


class TestZipatch(object):
    def create_patch_to_apply(self):
        tempname = tempfile.NamedTemporaryFile().name
        z = zipfile.ZipFile(tempname, 'w', zipfile.ZIP_DEFLATED)
        actions_json = json.dumps([
            {
                'type': 'store_file',
                'path': 'somedir/somefile.txt',
                'file': 'files/just a file in zipatch',
            },
            {
                'type': 'store_file',
                'path': 'somedir/somescript',
                'file': 'files/executable file in zipatch',
                'executable': True,
            },
            {
                'type': 'store_file',
                'path': 'somedir/not_a_script_any_more',
                'file': 'files/file with executable bit unset',
                'executable': False,
            },
            {
                'type': 'store_file',
                'path': 'new dir/new file',
                'file': 'files/file in a new directory',
            },
            {
                'type': 'store_file',
                'path': 'new dir/new script',
                'file': 'files/executable file in a new directory',
                'executable': True,
            },
            {
                'type': 'remove_file',
                'path': 'somedir/somefile_to_remove.txt',
            },
            {
                'type': 'remove_file',
                'path': 'somedir/nonexistent file',
            },
            {
                'type': 'remove_file',
                'path': 'nonexistent dir/nonexistent file',
            },
            {
                'type': 'remove_tree',
                'path': 'nonexistent dir',
            },
            {
                'type': 'remove_tree',
                'path': 'existing dir',
            },
            {
                'type': 'remove_tree',
                'path': 'existing file',
            },
        ])
        z.writestr('actions.json', actions_json)
        z.writestr('files/just a file in zipatch', 'some file content of a file in a directory')
        z.writestr('files/file in a new directory', 'some file content of a file in a new directory')
        z.writestr('files/executable file in zipatch', 'some executable content in a directory')
        z.writestr('files/file with executable bit unset', 'some content')
        z.writestr('files/executable file in a new directory', 'some executable content in a new directory')
        z.close()
        return tempname

    def test_apply_patch(self):
        p = self.create_patch_to_apply()
        z = Zipatch(p)

        assert z.base_svn_revision is None

        tempdir = tempfile.mkdtemp('rw')
        tempdir_ro = os.path.join(tempfile.gettempdir(), 'ro')
        os.makedirs(os.path.join(tempdir, 'somedir'))
        os.makedirs(os.path.join(tempdir, 'existing dir', 'nested dir'))
        open(os.path.join(tempdir, 'somedir', 'somefile.txt'), 'w')\
            .write('this content will be changed')

        open(os.path.join(tempdir, 'somedir', 'somescript'), 'w')\
            .write('this file will gain executable attribute')
        os.chmod(os.path.join(tempdir, 'somedir', 'somescript'), stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH)

        open(os.path.join(tempdir, 'somedir', 'not_a_script_any_more'), 'w')\
            .write('this file will lose executable attribute')
        os.chmod(os.path.join(tempdir, 'somedir', 'not_a_script_any_more'), stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)

        open(os.path.join(tempdir, 'somedir', 'somefile_to_remove.txt'), 'w')\
            .write('content of a file that should be deleted')
        open(os.path.join(tempdir, 'just a file in zipatch'), 'w')\
            .write('this file will not be changed')
        open(os.path.join(tempdir, 'existing file'), 'w')\
            .write('this is just a file')
        shutil.copytree(tempdir, tempdir_ro)

        z.apply(tempdir)
        z.apply(tempdir_ro, write_access=False)

        assert os.path.isdir(os.path.join(tempdir, 'new dir'))
        assert open(os.path.join(tempdir, 'new dir', 'new file')).read() == \
            'some file content of a file in a new directory'
        assert open(os.path.join(tempdir, 'new dir', 'new script')).read() == \
            'some executable content in a new directory'
        if not win.on_win():
            assert common.is_executable(os.path.join(tempdir, 'new dir', 'new script'))
            assert common.is_executable(os.path.join(tempdir_ro, 'new dir', 'new script'))

        assert not os.path.exists(os.path.join(tempdir, 'somedir', 'somefile_to_remove.txt'))
        assert open(os.path.join(tempdir, 'somedir', 'somefile.txt')).read() == \
            'some file content of a file in a directory'

        assert open(os.path.join(tempdir, 'somedir', 'somescript')).read() == \
            'some executable content in a directory'
        if not win.on_win():
            assert common.is_executable(os.path.join(tempdir, 'somedir', 'somescript'))
            assert common.is_executable(os.path.join(tempdir_ro, 'somedir', 'somescript'))
        assert open(os.path.join(tempdir, 'somedir', 'not_a_script_any_more')).read() == \
            'some content'
        if not win.on_win():
            assert not common.is_executable(os.path.join(tempdir, 'somedir', 'not_a_script_any_more'))
            assert not common.is_executable(os.path.join(tempdir_ro, 'somedir', 'not_a_script_any_more'))

        assert open(os.path.join(tempdir, 'just a file in zipatch')).read() == \
            'this file will not be changed'
        assert not os.path.exists(os.path.join(tempdir, 'existing dir'))
        assert not os.path.exists(os.path.join(tempdir, 'existing file'))

        shutil.rmtree(tempdir)

        os.remove(p)

    def test_paths(self):
        p = self.create_patch_to_apply()
        z = Zipatch(p)
        assert list(z.paths) == [
            'somedir/somefile.txt',
            'somedir/somescript',
            'somedir/not_a_script_any_more',
            'new dir/new file',
            'new dir/new script',
            'somedir/somefile_to_remove.txt',
            'somedir/nonexistent file',
            'nonexistent dir/nonexistent file',
            'nonexistent dir',
            'existing dir',
            'existing file',
        ]

    def create_patch_to_test(self):
        tmpfile = tempfile.NamedTemporaryFile(delete=False)
        tmpfile.write(b'tempname1 contents')
        tmpfile.flush()
        tmpfile.close()

        zipatchfile = tempfile.NamedTemporaryFile(delete=False)

        z = ZipatchWriter()
        z.add_action('store_file', 'some file', file=tmpfile.name)
        z.add_action('store_file', 'data file', data='embedded file data')
        z.add_action('store_file', 'executable file', data='embedded file data', executable=True)
        z.add_action('store_file', 'not executable file any more', data='embedded file data', executable=False)
        z.add_action('store_file', 'binary file', data='12@#$Dff', binary_hint=True)
        z.add_action('remove_file', 'some another file')
        z.add_action('remove_tree', 'some dir')
        z.add_action('svn_copy', 'some dst file', orig_path='/trunk/arcadia/some src file')
        z.add_action('store_file', 'some dst file', data='copied file data')
        z.add_action('svn_copy', 'some dst dir', orig_path='/trunk/arcadia/some src dir', orig_revision=123)
        z.add_action('mkdir', 'some dst dir')
        z.add_action('store_file', 'some dst dir/file 1', data='copied file 1 data')
        z.add_action('store_file', 'some dst dir/file 2', data='copied file 2 data')
        z.save(zipatchfile.name)
        zipatchfile.close()

        os.remove(tmpfile.name)

        return zipatchfile.name

    def test_create_patch(self):
        p = self.create_patch_to_test()

        z = zipfile.ZipFile(p, 'r')
        assert z.testzip() is None

        actions_json = z.read('actions.json')
        actions = json.loads(actions_json)
        assert actions == [
            {
                'type': 'store_file',
                'path': 'some file',
                'file': 'files/some file',
            },
            {
                'type': 'store_file',
                'path': 'data file',
                'file': 'files/data file',
            },
            {
                'type': 'store_file',
                'path': 'executable file',
                'file': 'files/executable file',
                'executable': True,
            },
            {
                'type': 'store_file',
                'path': 'not executable file any more',
                'file': 'files/not executable file any more',
                'executable': False,
            },
            {
                'type': 'store_file',
                'path': 'binary file',
                'file': 'files/binary file',
                'binary_hint': True,
            },
            {
                'type': 'remove_file',
                'path': 'some another file',
            },
            {
                'type': 'remove_tree',
                'path': 'some dir',
            },
            {
                'type': 'svn_copy',
                'path': 'some dst file',
                'orig_path': '/trunk/arcadia/some src file',
            },
            {
                'type': 'store_file',
                'path': 'some dst file',
                'file': 'files/some dst file',
            },
            {
                'type': 'svn_copy',
                'path': 'some dst dir',
                'orig_path': '/trunk/arcadia/some src dir',
                'orig_revision': 123,
            },
            {
                'type': 'mkdir',
                'path': 'some dst dir',
            },
            {
                'type': 'store_file',
                'path': 'some dst dir/file 1',
                'file': 'files/some dst dir/file 1',
            },
            {
                'type': 'store_file',
                'path': 'some dst dir/file 2',
                'file': 'files/some dst dir/file 2',
            },
        ]

        assert z.read('files/some file') == b'tempname1 contents'
        assert z.read('files/data file') == b'embedded file data'
        assert z.read('files/some dst file') == b'copied file data'
        assert z.read('files/some dst dir/file 1') == b'copied file 1 data'
        assert z.read('files/some dst dir/file 2') == b'copied file 2 data'
        z.close()

        os.remove(p)

    def test_full_duplicates(self):
        z = ZipatchWriter()
        z.add_action('store_file', 'some file', data='some data')
        z.add_action('store_file', 'some file', data='some data')

        assert z.actions == [
            {
                'path': 'some file',
                'type': 'store_file',
                'data': 'some data',
            }
        ]

        z.save(tempfile.mkstemp()[1])

        z.actions.append({'path': 'some file', 'type': 'store_file', 'data': 'some data'})
        with pytest.raises(ZipatchMalformedError) as e:
            z.save(tempfile.mkstemp()[1])
        assert e.value.args[0].endswith('already added to zipatch')

    def test_partial_duplicates(self):
        z = ZipatchWriter()
        z.add_action('store_file', 'some file', data='some data')

        with pytest.raises(ZipatchMalformedError) as e:
            z.add_action('store_file', 'some file', data='some other data')
        assert 'files content differs' in e.value.args[0]

        z.actions.append({'path': 'some file', 'type': 'store_file', 'data': 'some other data'})
        with pytest.raises(ZipatchMalformedError) as e:
            z.save(tempfile.mkstemp()[1])
        # Zipatch::save replaces 'data' field with 'file' pointing to the very same path, which yields error about full duplicate
        assert e.value.args[0].endswith('already added to zipatch')

        z = ZipatchWriter()
        z.add_action('store_file', 'some file', data='some data')
        with pytest.raises(ZipatchMalformedError) as e:
            z.add_action('store_file', 'some file', executable=True, data='some data')
        assert 'entries differ' in e.value.args[0]

        z.actions.append({'path': 'some file', 'type': 'store_file', 'data': 'some data', 'executable': True})
        with pytest.raises(ZipatchMalformedError) as e:
            z.save(tempfile.mkstemp()[1])
        assert e.value.args[0].startswith('Action "store_file" for path "some file" already added to zipatch but entries differ')

    def test_store_file_is_last(self):
        z = ZipatchWriter()
        z.add_action('svn_copy', 'some file', orig_path='file')
        z.add_action('store_file', 'some file', data='some data')

        with pytest.raises(ZipatchMalformedError) as e:
            z.add_action('remove_tree', 'some file')
        assert '"remove_tree" for path "some file" found after "store_file"' in e.value.args[0]

        z.actions.append({'path': 'some file', 'type': 'remove_tree'})
        z.add_action('store_file', 'some file', data='some data')

        with pytest.raises(ZipatchMalformedError) as e:
            z.save(tempfile.mkstemp()[1])
        assert '"remove_tree" for path "some file" found after "store_file"' in e.value.args[0]

    def test_store_after_copy(self):
        z = ZipatchWriter()
        z.add_action('svn_copy', 'some path', orig_path='path')
        z.add_action('store_file', 'some other file', data='some data')
        z.add_action('remove_tree', 'some another file', data='some data')

        with pytest.raises(ZipatchMalformedError) as e:
            z.save(tempfile.mkstemp()[1])
        assert '"store_file" or "mkdir" action not found after "svn_copy" action for path(s): "some path"' == e.value.args[0]

        z.add_action('mkdir', 'some path')
        z.save(tempfile.mkstemp()[1])

        z.actions[-1] = {'type': 'store_file', 'path': 'some path', 'data': 'some data'}
        z.save(tempfile.mkstemp()[1])

        z.add_action('svn_copy', 'some dir', orig_path='dir')
        z.add_action('mkdir', 'some dir')
        z.save(tempfile.mkstemp()[1])

        z.add_action('store_file', 'some dir/modified', data='data')
        z.add_action('remove_tree', 'some dir/deleted')
        z.add_action('mkdir', 'some dir/added')
        z.save(tempfile.mkstemp()[1])

        z.add_action('svn_copy', 'some dir/copied', orig_path='other dir')

        with pytest.raises(ZipatchMalformedError) as e:
            z.save(tempfile.mkstemp()[1])
        assert '"store_file" or "mkdir" action not found after "svn_copy" action for path(s): "some dir/copied"' == e.value.args[0]

    def test_base_svn_revision(self):
        path = tempfile.mkstemp()[1]
        z = ZipatchWriter()
        z.set_base_svn_revision('1234567')
        z.set_base_svn_revision(1234567)
        z.save(path)
        z = Zipatch(path)
        assert z.base_svn_revision == 1234567

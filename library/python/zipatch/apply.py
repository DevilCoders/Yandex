import errno
import json
import logging
import os
import shutil
import stat
import zipfile

from . import const
from . import misc
from . import validate

import library.python.path


logger = logging.getLogger(__name__)


class Zipatch(object):
    EXECUTABLE_MODE = stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH

    READONLY_MODE = stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH
    READ_EXEC_MODE = READONLY_MODE | EXECUTABLE_MODE
    READ_WRITE_MODE = READONLY_MODE | stat.S_IWUSR

    READ_WRITE_EXEC_MODE = READ_EXEC_MODE | stat.S_IWUSR

    def __init__(self, path):
        self.path = path

    def apply(self, dirpath, checkout_revision=None, write_access=True):
        logger.debug('Applying zipatch %s to directory %s maintaining write access %s', self.path, dirpath, write_access)
        self.write_access = write_access

        self.created_dirs = []

        with zipfile.ZipFile(self.path, 'r') as z:
            assert z.testzip() is None

            actions_json = z.read(const.ACTIONS_FILE)
            actions = json.loads(actions_json)
            files = []
            logger.debug('Zipatch %s contains %s actions', self.path, len(actions))

            for index, a in enumerate(actions):
                logger.debug('Applying %s-th action %s from zipatch %s', index, a, self.path)
                validate.validate_action(a, z)

                files.append(files.append(a['path']))

                real_path = os.path.join(dirpath, a['path'])

                t = a['type']
                if t == 'remove_file':
                    self.remove_file_handler(real_path)
                elif t == 'remove_tree':
                    self.remove_tree_handler(real_path)
                elif t == 'store_file':
                    self.store_file_handler(real_path, z.read(a['file']), a.get('executable', False), a.get('binary_hint', None), a.get('encrypted', None))
                elif t == 'svn_copy':
                    self.copy_handler(real_path, a['orig_path'], a.get('orig_revision', a.get('orig_rev')))
                elif t == 'mkdir':
                    self.mkdir_handler(real_path)

            def chmod(p, m):
                logger.debug('Changing access mode for %s to %s', p, m)
                try:
                    os.chmod(p, m)
                except Exception as e:
                    msg = 'Failed to chmod {} to {}'.format(p, m)
                    logger.exception(msg)
                    raise misc.ZipatchApplyError('{}: {}'.format(msg, repr(e)))

            # XXX Zipatch format has a contract that store_file (if present) is the last action for a file,
            # but it's unfortunately implicit. That's why we can't store files immediately in read-only mode.
            # TODO(workfork): make it better when zipatch format fixed
            if self.write_access:
                path_executablility = [(a['path'], a.get('executable', False)) for a in self.actions if a['type'] == 'store_file']
                logger.debug('Will set executable access for touched paths %s',
                             [path for path, exe in path_executablility if exe])
                logger.debug('Will unset executable access for touched paths %s',
                             [path for path, exe in path_executablility if not exe])
                for path, exe in set(path_executablility):
                    real_path = os.path.join(dirpath, path)
                    mode = self.READ_WRITE_EXEC_MODE if exe else self.READ_WRITE_MODE
                    chmod(real_path, mode)
            else:
                logger.debug('Will remove write access from touched paths')
                for a in reversed(sorted(self.actions, key=lambda k: k['path'])):
                    real_path = os.path.join(dirpath, a['path'])
                    if a['type'] == 'store_file':
                        mode = self.READ_EXEC_MODE if a.get('executable', False) else self.READONLY_MODE
                    elif a['type'] == 'mkdir':
                        mode = self.READ_EXEC_MODE
                    else:
                        continue
                    chmod(real_path, mode)
                logger.debug('Will remove write access from created dirs %s', self.created_dirs)
                for real_path in reversed(sorted(set(self.created_dirs))):
                    chmod(real_path, self.READ_EXEC_MODE)

        return files

    def remove_file_handler(self, path):
        logger.debug('Removing path %s', path)
        self._remove_path(path)

    def remove_tree_handler(self, path):
        logger.debug('Removing path %s', path)
        self._remove_path(path)

    def _remove_path(self, path):
        if os.path.exists(path) or os.path.lexists(path):
            if os.path.islink(path) or os.path.isfile(path):
                os.remove(path)
            else:
                shutil.rmtree(path)

    def store_file_handler(self, path, data, executable=False, binary_hint=None, encrypted=None):
        logger.debug('Writing to file %s %s bytes', path, len(data))

        dir = os.path.dirname(path)
        if not os.path.isdir(dir):
            self._makedirs(dir, mode=self.READ_WRITE_EXEC_MODE)
            logger.debug('Created directory %s', dir)

        self._remove_path(path)
        assert not os.path.exists(path)

        with open(path, 'wb', self.READ_WRITE_MODE) as f:
            f.write(data)

    def _makedirs(self, name, mode):
        head, tail = os.path.split(name)
        if not tail:
            head, tail = os.path.split(head)
        if head and tail and not os.path.exists(head):
            try:
                self._makedirs(head, mode)
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise
            if tail == os.curdir:
                return

        os.mkdir(name, mode)
        self.created_dirs.append(name)

    def copy_handler(self, path, orig_path, orig_revision=None):
        logger.debug('Copy path %s to %s - ignoring VCS specific action', orig_path, path)

    def mkdir_handler(self, path):
        logger.debug('Creating directory %s', path)
        self._remove_path(path)
        self._makedirs(path, mode=self.READ_WRITE_EXEC_MODE)

    def get_files_info(self):
        count = 0
        size = 0
        size_compressed = 0
        with zipfile.ZipFile(self.path, 'r') as z:
            for zinfo in z.infolist():
                if not zinfo.filename.startswith(const.FILES_DIR + '/'):
                    continue
                count += 1
                size += zinfo.file_size
                size_compressed += zinfo.compress_size
        return {
            'count': count,
            'size': size,
            'size_compressed': size_compressed,
        }

    @property
    def actions_data(self):
        with zipfile.ZipFile(self.path) as z:
            assert z.testzip() is None
            return z.read(const.ACTIONS_FILE)

    @property
    def meta_data(self):
        with zipfile.ZipFile(self.path) as z:
            assert z.testzip() is None
            try:
                return z.read(const.META_FILE)
            except KeyError:
                return None

    @property
    def meta(self):
        data = self.meta_data
        if data is None:
            return {}
        return json.loads(data)

    @property
    def actions(self):
        return json.loads(self.actions_data)

    @property
    def paths(self):
        return (a['path'] for a in self.actions)

    @property
    def normpaths(self):
        return tuple(sorted(set(list(os.path.normpath(a['path']) for a in self.actions))))

    @property
    def paths_common_prefix(self):
        return library.python.path.get_common_prefix(self.paths).form

    @property
    def base_svn_revision(self):
        return self.meta.get('base_svn_revision')

import subvertpy
import subvertpy.wc

import logging
import os

import library.python.svn_ssh
import vcs.svn.wc.client

from . import apply


logger = logging.getLogger(__name__)

SubversionException = subvertpy.SubversionException


class NotAWorkingCopyError(Exception):
    mute = True


class WorkingCopyTooOldError(Exception):
    mute = True


def check_working_copy(path):
    with vcs.svn.wc.client.SvnClient() as svn:
        try:
            svn.info(path)
        except SubversionException as ex:
            if ex.args[1] == 155036:
                raise WorkingCopyTooOldError(ex.args[0])
            else:
                raise
        if not subvertpy.wc.check_wc(path):
            raise NotAWorkingCopyError(path)


class ZipatchSvn(apply.Zipatch):

    def __init__(self, path):
        super(ZipatchSvn, self).__init__(path)

    def apply(self, dirpath, checkout_revision=None, write_access=True):
        if not write_access:
            logger.warn('%s always applies with write access', self.__class__.__name__)
        with vcs.svn.wc.client.SvnClient() as svn:
            _, info = svn.info(dirpath).popitem()

        self._repo_root = info.repos_root_url
        logger.debug('Repository root url for %s is %s', dirpath, self._repo_root)

        if checkout_revision:
            logger.debug('Checking out touched paths (%s) for base revision %s', len(self.normpaths), checkout_revision)
            with vcs.svn.wc.client.SvnClient() as svn, library.python.svn_ssh.ssh_multiplex(self._repo_root):
                for path in self.normpaths:
                    svn.update(os.path.join(dirpath, path), revision=checkout_revision, recurse=False, depth_is_sticky=False, make_parents=True)
        super(ZipatchSvn, self).apply(dirpath, write_access=True)

    def remove_file_handler(self, path):
        logger.debug('Removing path %s', path)
        self._remove_svn_path(path)

    def remove_tree_handler(self, path):
        logger.debug('Removing path %s', path)
        self._remove_svn_path(path)

    def _remove_svn_path(self, path):
        logger.debug('Removing path %s', path)
        try:
            with vcs.svn.wc.client.SvnClient() as svn:
                svn.delete([path.encode('utf8')], True, False, None)  # force=True, keep_local=False, py_revprops=None
        except SubversionException as ex:
            if ex.args[1] == 155007:  # 'is not under version control' or 'is not a working copy'
                logger.debug('Path %s is unversioned - removing from fs', path)
                super(ZipatchSvn, self)._remove_path(path)
            elif ex.args[1] not in [125001, 155010]:  # 'does not exists' or 'node was not found'
                raise

    def store_file_handler(self, path, data, executable=False, binary_hint=None, encrypted=None):
        super(ZipatchSvn, self).store_file_handler(path, data, executable, binary_hint, encrypted)
        logger.debug('Adding to svn file %s', path)
        with vcs.svn.wc.client.SvnClient() as svn:
            try:
                svn.add(path, add_parents=True)
            except SubversionException as ex:
                if ex.args[1] != 150002:  # 'is already under version control'
                    raise

            logger.debug('%s svn:executable property for %s', 'Setting' if executable else 'Deleting', path)
            # 'ON' is replaced by '*' by svn client for svn:executable property
            svn.propset('svn:executable', 'ON' if executable else None, path)

            if binary_hint is not None:
                logger.debug('%s svn:mime-type=application/octet-stream for %s', 'Setting' if binary_hint else 'Deleting', path)
                svn.propset('svn:mime-type', 'application/octet-stream' if binary_hint else None, path)

            if encrypted is not None:
                logger.debug('%s arc:encrypted property for %s', 'Setting' if encrypted else 'Deleting', path)
                svn.propset('arc:encrypted', '*' if encrypted else None, path)

    def copy_handler(self, path, orig_path, orig_revision=None):
        self._remove_svn_path(path)

        logger.debug('Copying path %s to %s from revision %s', orig_path, path, orig_revision)

        orig_url = subvertpy.wc.uri_canonicalize('/'.join((self._repo_root, orig_path)))
        logger.debug('Svn copy source is %s', orig_url)
        kwargs = {
            'src_path': orig_url,
            'dst_path': path,
            'make_parents': True,
        }

        if orig_revision is not None:
            kwargs['src_rev'] = int(orig_revision)

        with vcs.svn.wc.client.SvnClient() as svn:
            svn.copy(**kwargs)

    def mkdir_handler(self, path):
        logger.debug('Creating directory %s', path)
        with vcs.svn.wc.client.SvnClient() as svn:
            try:
                svn.mkdir([bytes(path.encode('utf-8'))], True)
            except SubversionException as ex:
                if ex.args[1] != 150002:  # 'is already under version control'
                    raise

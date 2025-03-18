import os
import collections
import subprocess as sp
import paths_tree
import logging
import xml.etree.ElementTree as etree


class Svn(object):

    def __init__(self, svn=None, login=None, key=None):
        self._svn = svn or 'svn'
        self._login = login
        self._key = key
        if key:
            try:
                os.chmod(key, 0400)
            except Exception as e:
                logging.warning(str(e))
        self._env = self._get_env()
        self._arc = 'svn+ssh://arcadia.yandex.ru/arc/'

    def _get_env(self):
        env = os.environ.copy()
        if self._login and self._key:
            env['SVN_SSH'] = 'ssh -l {} -i {}'.format(self._login, os.path.abspath(self._key))
        env['LC_CTYPE'] = 'en_US.UTF-8'
        env['LC_ALL'] = 'en_US.UTF-8'
        env['LANG'] = 'ru_RU.UTF-8'
        return env

    def _run(self, args):
        return sp.check_output([self._svn] + args, env=self._env)

    def svn_head(self):
        return int(etree.fromstring(self._run(['info', self._arc, '--xml'])).find('./entry').get('revision'))

    def revision_info(self, revision):
        log = etree.fromstring(self._run(['log', self._arc, '--xml', '-r' + str(revision)]))
        entry = log.find('./logentry')
        assert str(revision) == entry.get('revision')
        return {
            'revision': revision,
            'author': entry.find('./author').text if entry.find('./author') is not None else '',
            'date': entry.find('./date').text if entry.find('./date') is not None else '',
            'message': entry.find('./msg').text if entry.find('./msg') is not None else ''
        }

    def arcadia_diff_tree(self, revision1, revision2):
        diff = etree.fromstring(self._run(['diff', self._arc + 'trunk/arcadia/', '-r{}:r{}'.format(revision1, revision2), '--summarize', '--xml']))
        path_entries = collections.defaultdict(list)

        for el in diff.findall('./paths/path'):
            assert el.text is not None
            if not el.text.startswith(self._arc + 'trunk/arcadia/'):
                continue
            path = el.text[len(self._arc + 'trunk/arcadia/'):]
            path_entries[path].append(dict(el.items()))

        tree = paths_tree.PathsTree()
        for path in path_entries:
            tree.add_path(path, value=path_entries[path])

        return tree

    def export(self, src, dst, revision):
        if not os.path.exists(os.path.dirname(dst)):
            os.makedirs(os.path.dirname(dst))
        self._run(['export', self._arc + 'trunk/arcadia/' + src + '@' + str(revision), dst])

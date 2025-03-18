import json
import logging
import os
import six
import zipfile

from . import const
from . import validate

logger = logging.getLogger(__name__)


def _canonize_path(path):
    return os.path.normpath(path).replace('\\', '/')


def _debug_extra_params(entry):
    extra = entry.copy()
    extra.pop('type')
    extra.pop('path')
    if 'data' in extra:
        extra['data'] = len(extra['data'])
    return ', '.join('{}={}'.format(k, extra[k]) for k in sorted(six.iterkeys(extra)))


class ZipatchWriter(object):
    def __init__(self):
        self.meta = {}
        self.actions = []
        self.revprops = {}

    def init_from_svn_status_output(self, svn_status_output, root=None):
        """
        DEPRECATED: use add_by_svn_status_xml_output() instead
        plat text svn status is very bad to parse, use --xml
        """
        for line in svn_status_output.splitlines():
            (type, fullpath) = line.split(None, 1)

            relpath = fullpath if root is None else os.path.relpath(fullpath, root)

            if type in ['A', 'M']:
                if os.path.isfile(fullpath):
                    self.add_action('store_file', relpath, file=fullpath)
            if type in ['D']:
                self.add_action('remove_file', relpath)

    def init_from_zipatch(self, zipatch_path):
        z = zipfile.ZipFile(zipatch_path, 'r')

        actions_json = z.read(const.ACTIONS_FILE)
        actions = json.loads(actions_json)
        for a in actions:
            data = z.read(a.pop('file')) if a.get('file') else None
            action_type = a.pop('type')
            action_path = a.pop('path')
            self.add_action(action_type, action_path, data=data, **a)

        try:
            meta_json = z.read(const.META_FILE)
        except KeyError:
            meta_json = None
        if meta_json:
            self.meta.update(json.loads(meta_json))

        try:
            revprops_json = z.read(const.REVPROPS_FILE)
        except KeyError:
            revprops_json = None
        if revprops_json:
            self.revprops.update(json.loads(revprops_json))

        z.close()

    def add_action(self, action_type, path, **params):
        assert action_type in const.ACTION_HANDLERS

        canon_path = _canonize_path(path).strip('/')
        entry = {k: v for (k, v) in six.iteritems(params) if v is not None}
        entry.update({
            'type': action_type,
            'path': canon_path,
        })

        assert not ('file' in entry and 'data' in entry)

        if 'file' in entry:
            fs_path = entry.pop('file')
            with open(fs_path, 'rb') as f:
                logger.debug('Fetching file contents from %s', fs_path)
                entry['data'] = f.read()

        if validate.action_duplicates(self.actions, action=entry, raise_full_dups=False):
            logger.debug('Action %s path=%s, %s already added to zipatch - ignoring', entry.get('type'), entry.get('path'), _debug_extra_params(entry))
        else:
            self.actions.append(entry)
            logger.debug('Added action %s path=%s, %s', entry.get('type'), entry.get('path'), _debug_extra_params(entry))

    def set_base_svn_revision(self, revision):
        self.meta['base_svn_revision'] = int(revision)

    def add_revprop(self, key, value):
        self.revprops[key] = value
        logger.debug('Added revprop %s = %s', key, value)

    def save(self, file):
        logger.debug('Saving zipatch to file %s', file)

        z = zipfile.ZipFile(file, 'w', zipfile.ZIP_DEFLATED)
        finalized_actions = []

        for a in self.actions:
            a = a.copy()

            # Integrate file data
            assert 'file' not in a
            if 'data' in a:
                file_payload = a.pop('data')
                a['file'] = '/'.join((const.FILES_DIR, a['path']))
                logger.debug('Adding data %s to zipatch as %s', len(file_payload), a['file'])
                z.writestr(a['file'], file_payload)

            finalized_actions.append(a)

        z.writestr(const.ACTIONS_FILE, json.dumps(finalized_actions, indent=4))

        if self.meta:
            z.writestr(const.META_FILE, json.dumps(self.meta, sort_keys=True, indent=4))

        if self.revprops:
            z.writestr(const.REVPROPS_FILE, json.dumps(self.revprops, sort_keys=True, indent=4))

        validate.validate_zip(z)

        z.close()

    @property
    def paths(self):
        return (act['path'] for act in self.actions)

    @property
    def normpaths(self):
        return tuple(sorted(set(list(os.path.normpath(a['path']) for a in self.actions))))

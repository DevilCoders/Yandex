import copy
import json
import logging

from . import const

logger = logging.getLogger(__name__)

META_KEYS = frozenset(('base_svn_revision',))


class ZipatchMalformedError(Exception):
    pass


def validate_zip(z):
    assert z.testzip() is None
    actions = json.loads(z.read(const.ACTIONS_FILE))

    try:
        meta = json.loads(z.read(const.META_FILE))
    except KeyError:
        meta = None
    if meta is not None:
        assert isinstance(meta, dict)
        assert all(key in META_KEYS for key in meta)
        svn_base_revision = meta.get('base_svn_revision')
        if svn_base_revision is not None:
            assert isinstance(svn_base_revision, int)

    copy_files = set()
    copy_dirs_empty = set()
    for index, a in enumerate(actions):
        validate_action(a, z)
        action_duplicates(actions, action_idx=index)

        # here we can't validate that all subpaths of copied dirs are listed somehow in zipatch
        for copy_parent in copy_dirs_empty.copy():
            if a['path'].startswith(copy_parent):
                copy_dirs_empty.remove(copy_parent)

        if a['type'] == 'svn_copy':
            copy_files.add(a['path'])
        elif a['path'] in copy_files:
            if a['type'] == 'store_file':
                copy_files.remove(a['path'])
            elif a['type'] == 'mkdir':
                copy_files.remove(a['path'])
                copy_dirs_empty.add(a['path'])

    if copy_dirs_empty:
        logger.info('Seems that you copied an empty directory(s) "%s". If it\'s not true, please contact devtools@.',
                    '", "'.join(list(copy_dirs_empty)))

    if copy_files:
        raise ZipatchMalformedError(
            '"store_file" or "mkdir" action not found after "svn_copy" action for path(s): "{}"'.format(
                '", "'.join(list(copy_files))
            ))


def validate_action(action, z):
    try:
        assert 'type' in action, 'Action does not contain type: {}'.format(action)
        t = action['type']

        assert t in const.ACTION_HANDLERS, 'Unknown action type: {}'.format(t)

        assert 'path' in action, 'Action does not contain path: {}'.format(action)
        path = action['path']

        if t == 'store_file':
            assert 'file' in action, 'Action {} for {} does not contain "file" attribute'.format(t, path)
            source_file = action['file']

            assert source_file.startswith(const.FILES_DIR), 'Action {} for {}: "file" attribute must start with "{}"'.format(t, path, const.FILES_DIR)
            try:
                z.getinfo(source_file)
            except KeyError:
                raise ZipatchMalformedError('Action {} for {}: path in "file" attribute {} not present in zipatch'.format(t, path, source_file))
            except:
                raise

        if t == 'svn_copy':
            assert action.get('orig_path'), 'Action {} for {} must contain non empty "orig_path" attribute'.format(t, path)

    except AssertionError as ex:
        raise ZipatchMalformedError(ex.message)


def action_duplicates(actions, action=None, action_idx=None, raise_full_dups=True):

    if action_idx is None:
        action_idx = len(actions)
    else:
        action = actions[action_idx]

    action_type = action['type']
    action_path = action['path']

    full_dup_found = False

    try:
        dups = [a for (i, a) in enumerate(actions) if a['type'] == action_type and a['path'] == action_path and i != action_idx]
        for dup in dups:
            if dup != action:

                action_copy = copy.deepcopy(action)
                dup_copy = copy.deepcopy(dup)

                assert action_copy.pop('data', '') == dup_copy.pop('data', ''), 'Action "{}" for path "{}" already added to zipatch but files content differs'.format(
                    action_type, action_path)
                assert action_copy == dup_copy, 'Action "{}" for path "{}" already added to zipatch but entries differ:\nFirst entry: {}\nSecond entry: {}'.format(
                    action_type, action_path, dup_copy, action_copy)

            elif raise_full_dups:
                raise ZipatchMalformedError('Action "{}" already added to zipatch'.format(action))
            else:
                full_dup_found = True

        if not full_dup_found and action_type != 'store_file':
            dups = [a for a in actions[:action_idx] if a['path'] == action_path]
            for dup in dups:
                assert dup['type'] != 'store_file', 'Action "{}" for path "{}" found after "store_file" which must be the last one\n{}'.format(action_type, action_path, actions)

        return full_dup_found
    except AssertionError as ex:
        raise ZipatchMalformedError(ex.args[0])

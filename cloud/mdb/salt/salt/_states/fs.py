# -*- coding: utf-8 -*-
"""
States for managing filesystem and its objects.
"""

from __future__ import unicode_literals

import logging
import os

import yaml

__opts__ = {}
__salt__ = {}
__states__ = {}

log = logging.getLogger(__name__)


def __virtual__():
    return True


def file_present(
    name,
    s3_cache_path=None,
    contents=None,
    contents_function=None,
    contents_function_args=None,
    contents_function_args_from_pillar=None,
    contents_format=None,
    url=None,
    use_service_account_authorization=False,
    decode_contents=False,
    s3_bucket=None,
    **kwargs
):
    """
    Like `file.managed`, but with ability to invoke a function and use its result as file contents.
    """
    cached = False
    if contents_function:
        function_kwargs = contents_function_args or {}

        for arg_name, pillar_key in (contents_function_args_from_pillar or {}).items():
            function_kwargs[arg_name] = __salt__['pillar.get'](pillar_key)

        contents = __salt__[contents_function](**function_kwargs)
    elif url:
        if s3_cache_path:
            s3_client = __salt__['mdb_s3.client']()
            if not __salt__['mdb_s3.object_exists'](s3_client, s3_cache_path, s3_bucket=s3_bucket):
                contents = __salt__['fs.download_object'](url, use_service_account_authorization)
                if not __opts__['test']:
                    __salt__['mdb_s3.create_object'](s3_client, s3_cache_path, contents, s3_bucket=s3_bucket)
                    cached = True
            else:
                contents = __salt__['mdb_s3.get_object'](s3_client, s3_cache_path, s3_bucket=s3_bucket)
        else:
            contents = __salt__['fs.download_object'](url, use_service_account_authorization)
    if contents_format:
        if contents_format == 'yaml':
            contents = yaml.safe_dump(contents, default_flow_style=False, indent=4, allow_unicode=True)
        else:
            return _error(name, 'Unsupported contents_format: {0}'.format(contents_format))
    else:
        if decode_contents and isinstance(contents, (bytes, bytearray)):
            contents = contents.decode("utf-8")

    result = __states__['file.managed'](name=name, contents=contents, **kwargs)
    if cached:
        result['changes']['cached'] = 'object cached in s3 path: "{}"'.format(s3_cache_path)
    return result


def directory_cleanedup(name, expected, prefix='', suffix=''):
    """
    Clean up the directory from unexpected files.
    """
    try:
        if not os.path.exists(name):
            return {'name': name, 'changes': {}, 'result': True, 'comment': 'Directory {0} does not exist'.format(name)}

        expected_files = set('{0}{1}{2}'.format(prefix, entry, suffix) for entry in expected)
        unexpected_files = set()
        for file in os.listdir(name):
            if not file.startswith(prefix):
                continue

            if not file.endswith(suffix):
                continue

            if file in expected_files:
                continue

            unexpected_files.add(file)

        if not unexpected_files:
            return {
                'name': name,
                'changes': {},
                'result': True,
                'comment': 'Directory {0} does not require any changes'.format(name),
            }

        unexpected_paths = [os.path.join(name, f) for f in unexpected_files]
        changes = {'removed': '\n'.join(unexpected_paths)}

        if __opts__['test']:
            return {
                'name': name,
                'changes': changes,
                'result': None,
                'comment': '\n'.join('{0} will be removed'.format(p) for p in unexpected_paths),
            }

        for path in unexpected_paths:
            os.remove(path)

        return {
            'name': name,
            'changes': changes,
            'result': True,
            'comment': '\n'.join('{0} was removed'.format(p) for p in unexpected_paths),
        }

    except Exception as e:
        log.exception('failed to clean up directory:')
        return _error(name, str(e))


def _error(name, comment):
    return {'name': name, 'changes': {}, 'result': False, 'comment': comment}

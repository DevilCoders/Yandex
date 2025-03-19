# -*- coding: utf-8 -*-
"""
States for managing Zookeeper.
"""

from __future__ import unicode_literals

import traceback
import subprocess
import os.path
import re

__opts__ = {}
__salt__ = {}
__states__ = {}


def __virtual__():
    return True


def config_rendered(name, params=None, nodes=None, zk_users=None, **kwargs):
    """
    Render zoo.cfg.
    """
    ret = {'name': name, 'result': False, 'changes': {}, 'comment': ''}
    test = __opts__['test']
    try:
        config_old = __salt__['mdb_zookeeper.load_config'](name)
        config_new = __salt__['mdb_zookeeper.generate_config'](config_old, params, nodes, zk_users)
    except Exception:
        ret['result'] = False
        ret['comment'] = traceback.format_exc()
        return ret

    changes = __salt__['mdb_zookeeper.compare_config'](config_new, config_old)
    if not changes:
        ret['result'] = True
        return ret

    ret['changes'] = changes
    if test:
        ret['result'] = None
        return ret

    contents = __salt__['mdb_zookeeper.render_config'](config_new)
    fm_ret = __states__['file.managed'](name, contents=contents, **kwargs)

    ret['result'] = fm_ret['result']
    ret['comment'] = fm_ret['comment']

    return ret


def _get_keystore_hashes(keystore, password):
    out_keytool = subprocess.Popen(
        ['keytool', '-list', '-v', '-storepass', password, '-keystore', keystore],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    out = ""
    lines = out_keytool.communicate()[0].splitlines()
    for line in lines:
        line_str = line.decode("utf-8")
        if 'SHA256:' in line_str:
            out += line_str
    return out


def _compare_keystores(keystore1, keystore2, password):
    """
    Compare two keystores by cert hashes
    Keystores cannot be compare as binary files
    """
    hashes1 = _get_keystore_hashes(keystore1, password)
    hashes2 = _get_keystore_hashes(keystore2, password)
    return hashes1 != hashes2


def _ensure_file(name, cmds_gen, cmds_fin, file_new, file_old, compare_files, can_generate):
    """
    Generate some file, compare with old and replace if some diffs
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }

    diff = not os.path.isfile(file_old)

    # Do not try generate file for test purpose when impossible
    must_generate = can_generate and (not __opts__['test'] or not diff)

    if must_generate:
        # Execute command set for generate file
        for cmd_opts in cmds_gen:
            cmd, opts = cmd_opts
            res = __salt__['cmd.retcode'](cmd, **opts)
            ret['comment'].append('"{cmd}" exited with {res}'.format(cmd=cmd, res=res))
            if res != 0:
                ret['result'] = False
                return ret
    else:
        diff = True

    if not __opts__['test']:
        if not os.path.isfile(file_new):
            ret['comment'].append('Cannot generate file "{file_new}"'.format(file_new=file_new))
            ret['result'] = False
            return ret

    if not diff:
        diff = compare_files(file_new, file_old)

    if not diff:
        # Cleanup if files equals
        ret['result'] = True
        ret['comment'] = []
        cmd = 'rm -f {file_new}'.format(file_new=file_new)
        res = subprocess.call(cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        if res:
            ret['comment'].append('"{cmd}" exited with {res}'.format(cmd=cmd, res=res))
        return ret

    if __opts__['test']:
        ret['comment'] = []
        for cmd_opts in cmds_gen:
            ret['comment'].append('{cmd} would be executed'.format(cmd=cmd_opts[0]))
        for cmd_opts in cmds_fin:
            ret['comment'].append('{cmd} would be executed'.format(cmd=cmd_opts[0]))
        ret['changes'] = {name: 'pending execution'}
        ret['result'] = None
    else:
        # Execute final commands set
        for cmd_opts in cmds_fin:
            cmd, opts = cmd_opts
            res = __salt__['cmd.retcode'](cmd, **opts)
            ret['comment'].append('"{cmd}" exited with {res}'.format(cmd=cmd, res=res))
            if res != 0:
                ret['result'] = False
                return ret
        ret['changes'] = {name: 'executed'}
        ret['result'] = True

    return ret


def _get_password_from_config(config, section):
    name = 'ssl.{section}.password'.format(section=section)
    fields = __salt__['mdb_zookeeper.load_config'](config)
    if not name in fields:
        if __opts__['test']:
            return 'DummyPassword'
        raise Exception("Can't find {section} password in {config}".format(section=section, config=config))
    return fields[name]


def _ensure_keystore(name, path, host, user, group, mode, config):
    """
    Convert server.key to Java keystore
    """
    password = _get_password_from_config(config, "keyStore")

    cmds_gen = []
    cmds_gen.append(['rm -f {path}/server.jks.tmp'.format(path=path), {}])
    cmds_gen.append(
        [
            'openssl pkcs12 -export -in {path}/server.crt -inkey {path}/server.key -out {path}/server.p12 -name {host} -passout pass:{password}'.format(
                path=path, host=host, password=password
            ),
            {},
        ]
    )
    cmds_gen.append(
        [
            'keytool -importkeystore -destkeystore {path}/server.jks.tmp -srckeystore {path}/server.p12 -deststorepass {password} -srcstoretype PKCS12 -srcstorepass {password} -alias {host}'.format(
                path=path, host=host, password=password
            ),
            {},
        ]
    )
    cmds_gen.append(['rm -f {path}/server.p12'.format(path=path), {}])

    cmds_fin = []
    cmds_fin.append(['mv {path}/server.jks.tmp {path}/server.jks'.format(path=path), {}])
    cmds_fin.append(['chown {user}:{group} {path}/server.jks'.format(user=user, group=group, path=path), {}])
    cmds_fin.append(['chmod {mode} {path}/server.jks'.format(mode=mode, path=path), {}])

    def compare_keystores_nopwd(keystore1, keystore2):
        return _compare_keystores(keystore1, keystore2, password)

    can_generate = os.path.isfile('{path}/server.crt'.format(path=path))

    ret = _ensure_file(
        name,
        cmds_gen,
        cmds_fin,
        '{path}/server.jks.tmp'.format(path=path),
        '{path}/server.jks'.format(path=path),
        compare_keystores_nopwd,
        can_generate,
    )
    for i, comment in enumerate(ret['comment']):
        ret['comment'][i] = comment.replace(password, '<*password*>')
    return ret


def ensure_keystore(name, path, host, user, group, mode, config, **kwargs):
    try:
        return _ensure_keystore(name, path, host, user, group, mode, config)
    except Exception:
        ret = {'name': name, 'result': False, 'changes': {}, 'comment': traceback.format_exc()}
        return ret


def _ensure_truststore(name, path, host, user, group, mode, config, **kwargs):
    """
    Convert allCAs.pem to Java truststore
    """
    password = _get_password_from_config(config, "trustStore")

    cmds_gen = []
    cmds_gen.append(['rm -f {path}/truststore.jks.tmp'.format(path=path), {}])
    cmds_gen.append(
        [
            'keytool -import -trustcacerts -alias yandex -file {path}/allCAs.pem -keystore {path}/truststore.jks.tmp -storepass {password} -noprompt'.format(
                path=path, host=host, password=password
            ),
            {},
        ]
    )

    cmds_fin = []
    cmds_fin.append(['mv {path}/truststore.jks.tmp {path}/truststore.jks'.format(path=path), {}])
    cmds_fin.append(['chown {user}:{group} {path}/truststore.jks'.format(user=user, group=group, path=path), {}])
    cmds_fin.append(['chmod {mode} {path}/truststore.jks'.format(mode=mode, path=path), {}])

    def compare_keystores_nopwd(keystore1, keystore2):
        return _compare_keystores(keystore1, keystore2, password)

    can_generate = os.path.isfile('{path}/allCAs.pem'.format(path=path))

    ret = _ensure_file(
        name,
        cmds_gen,
        cmds_fin,
        '{path}/truststore.jks.tmp'.format(path=path),
        '{path}/truststore.jks'.format(path=path),
        compare_keystores_nopwd,
        can_generate,
    )
    for i, comment in enumerate(ret['comment']):
        ret['comment'][i] = comment.replace(password, '<*password*>')
    return ret


def ensure_truststore(name, path, host, user, group, mode, config, **kwargs):
    try:
        return _ensure_truststore(name, path, host, user, group, mode, config)
    except Exception:
        ret = {'name': name, 'result': False, 'changes': {}, 'comment': traceback.format_exc()}
        return ret

"""
Simple gpg mock
"""

from .utils import handle_action


def _set_str(key, name):
    key.__str__ = lambda *args, **kwargs: name


def gen_uid(state, name):
    """
    Uidgen mock
    """
    action_id = f'gpg-keygen-{name}'
    handle_action(state, action_id)
    return f'gpg key for {name}'


def gpg(mocker, state):
    """
    Setup gpg mock
    """
    pgpkey = mocker.patch('pgpy.PGPKey.new')
    pgpkey.return_value.add_uid.side_effect = lambda name, **_: _set_str(pgpkey.return_value, name)
    pgpkey.return_value.fingerprint.keyid = '0'
    pgpuid = mocker.patch('pgpy.PGPUID.new')
    pgpuid.side_effect = lambda name, **_: gen_uid(state, name)

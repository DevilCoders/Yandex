"""
Simple nacl mock
"""

from .utils import handle_action


def encrypt(state, data):
    """
    Encryption mock
    """
    action_id = f'nacl-encrypt-{data.decode("utf-8")}'
    handle_action(state, action_id)
    return 'encrypted '.encode('utf-8') + data


def decrypt(state, data):
    """
    Decryption mock
    """
    action_id = f'nacl-decrypt-{data}'
    handle_action(state, action_id)
    return f'decrypted {data}'.encode('utf-8')


def nacl(mocker, state):
    """
    Setup nacl mock
    """
    public_key = mocker.patch('cloud.mdb.dbaas_worker.internal.crypto.PublicKey')
    public_key.side_effect = lambda key, _: key.decode('utf-8')
    secret_key = mocker.patch('cloud.mdb.dbaas_worker.internal.crypto.PrivateKey')
    secret_key.side_effect = lambda key, _: key.decode('utf-8')
    encoder = mocker.patch('cloud.mdb.dbaas_worker.internal.crypto.encoder')
    encoder.encode.side_effect = lambda value: value
    encoder.decode.side_effect = lambda value: value
    random = mocker.patch('cloud.mdb.dbaas_worker.internal.crypto.random')
    random.return_value = True
    box = mocker.patch('cloud.mdb.dbaas_worker.internal.crypto.Box')
    box.return_value.encrypt.side_effect = lambda data, _: encrypt(state, data)
    box.return_value.decrypt.side_effect = lambda data: decrypt(state, data)

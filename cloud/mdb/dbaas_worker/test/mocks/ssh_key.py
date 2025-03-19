"""
Simple ssh_key mock
"""


def ssh_key(mocker, _):
    """
    Setup ssh_key mock
    """
    mocker.patch('cloud.mdb.dbaas_worker.internal.providers.ssh_key.default_backend')
    rsa = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.ssh_key.rsa')
    rsa.generate_private_key.return_value.private_bytes.return_value = 'ssh-private'.encode('utf-8')
    rsa.generate_private_key.return_value.public_key.return_value.public_bytes.return_value = 'ssh-public'.encode(
        'utf-8'
    )

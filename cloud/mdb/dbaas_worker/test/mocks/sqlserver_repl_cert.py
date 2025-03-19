"""
Simple sqlserver_repl_cert mock
"""


def sqlserver_repl_cert(mocker, _):
    """
    Setup sqlserver_repl_cert mock
    """
    mocker.patch('cloud.mdb.dbaas_worker.internal.providers.sqlserver_repl_cert.default_backend')
    rsa = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.sqlserver_repl_cert.rsa')
    rsa.generate_private_key.return_value.private_bytes.return_value = 'ss-repl-secret'.encode('utf-8')
    rsa.generate_private_key.return_value.public_key.return_value.public_bytes.return_value = 'ss-repl-public'.encode(
        'utf-8'
    )
    serial = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.sqlserver_repl_cert.random_serial_number')
    serial.return_value = 'ss-repl-serial'
    builder = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.sqlserver_repl_cert.CertificateBuilder'
    ).return_value
    builder.subject_name.return_value = builder
    builder.issuer_name.return_value = builder
    builder.public_key.return_value = builder
    builder.serial_number.return_value = builder
    builder.not_valid_before.return_value = builder
    builder.not_valid_after.return_value = builder
    builder.sign.return_value = builder
    builder.public_bytes.return_value = 'ss-repl-cert'.encode('utf-8')

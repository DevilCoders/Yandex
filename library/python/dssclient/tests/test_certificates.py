from datetime import datetime
from dssclient.resources.certificate import Certificate, CertificateRequest


def test_basic(read_fixture):
    cert_data = read_fixture('certificates.json')[0]
    cert = Certificate.spawn(cert_data, endpoint=None)

    assert cert.parsed.public_key_base64 == (
        'BEAT6hW+sqlYmtBIgBuwj9BcfNI4A4DgTnH1eZqlB4d1si6YIE6IyFEOdxg6IRYONP7u8xpuz3tBzcGWtj/4yEnE')
    assert cert.parsed.subject['country'] == 'RU'
    assert str(cert.parsed.date_issued) == '2017-09-25 12:25:11'
    assert str(cert.parsed.date_expires) == '2018-12-25 12:35:11'

    assert not cert.parsed.check_valid_on_date(datetime(2017, 9, 25, 12, 25, 10))
    assert cert.parsed.check_valid_on_date(datetime(2017, 9, 25, 12, 25, 11))
    assert not cert.parsed.check_valid_on_date(datetime(2018, 12, 25, 12, 35, 11))
    assert not cert.parsed.check_valid_on_date(datetime(2018, 12, 25, 12, 35, 12))

    assert '2' in f'{cert}'
    assert cert.id == 2
    assert not cert.is_default
    assert cert.authority_id == 8
    assert 'MIID3' in cert.body_base64
    assert cert.csp_id == '7a76f462-4204-4282-a6d5-687774bfb49f'
    assert 'L=Novosibirsk, S=Novosibirsk, C=RU' in cert.subject
    assert '090ee45d1500d380e711eda1332bf5f7' in cert.serial
    assert cert.body_bytes
    assert cert.status_code == 'OUT_OF_ORDER'

    assert cert.is_active
    assert not cert.is_inactive

    assert cert.status_is_read_only
    assert not cert.status_is_valid
    assert not cert.status_is_invalid
    assert not cert.status_is_revoked
    assert not cert.status_is_suspended
    assert 'ID' in cert.pformat()
    assert isinstance(cert.asdict(), dict)


def test_get_all(fake_dss, read_fixture):

    dss = fake_dss(read_fixture('certificates.json'))
    certs = dss.certificates.get_all()

    assert len(certs) == 1
    assert certs[0].id == 2


def test_setters(fake_dss):

    dss = fake_dss()

    cert = dss.certificates.get(2, refresh=False)
    assert cert.set_pin('1234')


def test_requests_basic(read_fixture):
    requests_data = read_fixture('certificate_requests.json')[0]
    req = CertificateRequest.spawn(requests_data, endpoint=None)

    assert '4' in f'{req}'
    assert req.id == 4
    assert req.status_is_processed
    assert not req.status_is_pending
    assert not req.status_is_registering
    assert not req.status_is_rejected
    assert 'L=Nsk, O=Yandex, OU=Fin, E=robot-bcl@yandex-team.ru' in req.subject
    assert req.type_is_issue
    assert not req.type_is_revoke
    assert req.authority_id == 2
    assert req.authority_request_id == 'f7b9e5ef-c925-e811-80d7-00155d454d12'
    assert req.authority_title is None
    assert req.certificate_id == 2
    assert req.common_name == 'Робот BCL'
    assert req.body_bytes
    assert req.body_base64.startswith('MIICGDCCAcc')


def test_requests_get_all(fake_dss, read_fixture):
    dss = fake_dss(read_fixture('certificate_requests.json'))
    requests = dss.certificates.requests.get_all()

    assert len(requests) == 2
    assert requests[0].id == 4
    assert requests[1].id == 12

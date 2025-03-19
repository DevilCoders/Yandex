import pytest
from cloud.mdb.salt.salt._modules import certificate_manager


certificate_manager.__opts__ = {
    'certificate-manager': {
        'address': 'certificate-manager.test',
    }
}


_RESP = '''
{
  "certificate_id": "cert-id",
  "version_id": "cert-ver",
  "certificate_chain": [
    "CERTIFICATE1",
    "CERTIFICATE2",
    "CERTIFICATE3"
  ],
  "private_key": "PRIVATE KEY"
}
'''


def test_get_happy_path():
    def cmd_run(*args, **kwargs):
        return {
            'retcode': 0,
            'stderr': '',
            'stdout': _RESP,
        }

    certificate_manager.__salt__['cmd.run_all'] = cmd_run
    certificate_manager.__salt__['compute_metadata.iam_token'] = lambda: '42'
    certificate_manager.__salt__['compute_metadata.attribute'] = lambda attr: attr
    resp = certificate_manager.get()
    assert resp == {
        'cert.key': "PRIVATE KEY",
        'cert.ca': [
            "CERTIFICATE2",
            "CERTIFICATE3",
        ],
        'cert.crt': ["CERTIFICATE1", "CERTIFICATE2", "CERTIFICATE3"],
    }


def test_get_fails():
    def cmd_run(*args, **kwargs):
        return {
            'retcode': 1,
            'stderr': '...',
            'stdout': '',
        }

    certificate_manager.__salt__['cmd.run_all'] = cmd_run
    certificate_manager.__salt__['compute_metadata.iam_token'] = lambda: '42'
    certificate_manager.__salt__['compute_metadata.attribute'] = lambda attr: attr
    with pytest.raises(certificate_manager.CommandExecutionError):
        certificate_manager.get()

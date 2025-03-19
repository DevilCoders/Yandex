from cloud.mdb.salt.salt._modules import lockbox
from cloud.mdb.internal.python.pytest.utils import parametrize


lockbox.__opts__ = {
    'lockbox': {
        'address': 'lockbox.test',
    }
}


@parametrize(
    {
        'id': 'text-secret',
        'args': {
            'lockbox_response': """
                {
                "entries": [
                    {
                    "key": "test-key",
                    "text_value": "test-value"
                    }
                ],
                "version_id": "test-version"
                }
            """,
            'secret': {'test-key': 'test-value'},
        },
    },
    {
        'id': 'binary-secret',
        'args': {
            'lockbox_response': """
                {
                "entries": [
                    {
                    "key": "test-key",
                    "binary_value": "dGVzdC1iaW5hcnktdmFsdWU="
                    }
                ],
                "version_id": "test-version"
                }
            """,
            'secret': {'test-key': 'test-binary-value'},
        },
    },
)
def test_text_secret(lockbox_response, secret):
    def cmd_run(*args, **kwargs):
        return {
            'retcode': 0,
            'stderr': '',
            'stdout': lockbox_response,
        }

    lockbox.__salt__['cmd.run_all'] = cmd_run
    lockbox.__salt__['compute_metadata.iam_token'] = lambda: '42'
    assert lockbox.get('foo') == secret

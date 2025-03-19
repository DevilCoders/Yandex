import json
from collections import namedtuple


SecretsBase = namedtuple(
    'Secrets',
    field_names=[
        'yt_token',
        'mds_static_access_key',
        'mds_static_secret_key',
        'mds_service_id',
        'registry_key_string'
    ]
)


class Secrets(SecretsBase):
    def __new__(
        cls,
        yt_token,
        mds_static_access_key,
        mds_static_secret_key,
        mds_service_id,
        registry_key_path
    ):
        with open(registry_key_path, 'r') as f:
            registry_key_string = json.load(f)
        return super(Secrets, cls).__new__(
            cls,
            yt_token,
            mds_static_access_key,
            mds_static_secret_key,
            mds_service_id,
            registry_key_string
        )

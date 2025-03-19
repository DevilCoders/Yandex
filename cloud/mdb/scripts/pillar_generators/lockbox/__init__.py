import logging
import subprocess
import json
from typing import NamedTuple

log = logging.getLogger(__name__)


class TextValue(NamedTuple):
    key: str
    value: str


class BinaryValue(NamedTuple):
    key: str
    value: str


class LockBox:
    def __init__(self, ycp_profile, folder_id):
        self.ycp_profile = ycp_profile
        self.folder_id = folder_id

    def _call(self, lb_cmd, **kwargs):
        if 'input' in kwargs:
            log.info('input = %s', kwargs['input'])
            kwargs['input'] = kwargs['input'].encode('utf-8')
        cmd = ['ycp', '--format=json', f'--profile={self.ycp_profile}', 'lockbox', 'v1'] + list(lb_cmd)
        log.info('cmd: %s', ' '.join(cmd))
        out = subprocess.check_output(cmd, **kwargs)
        return json.loads(out)

    def create_secret(self, name, *values):
        values_args = []
        for val in values:
            kind = 'text-value' if isinstance(val, TextValue) else 'binary-value'
            values_args += ['--version-payload-entries', f'key={val.key},{kind}={val.value}']
        return self._call(
            [
                'secret',
                'create',
                f'--name={name}',
                f'--folder-id={self.folder_id}',
            ]
            + values_args,
        )['id']

    def add_access_binding(self, secret_id, service_account_id):
        request = json.dumps(
            {
                "resource_id": secret_id,
                "private_call": True,
                "access_binding_deltas": [
                    {
                        "action": "ADD",
                        "access_binding": {
                            "role_id": "lockbox.payloadViewer",
                            "subject": {"id": service_account_id, "type": "serviceAccount"},
                        },
                    }
                ],
            }
        )
        return self._call(['secret', 'update-access-bindings', '--request', '-'], input=request)

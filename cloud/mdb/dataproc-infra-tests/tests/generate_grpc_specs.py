import logging
import os
import shlex
import subprocess
from .helpers.utils import get_env_without_python_entry_point


def main():
    if os.path.exists('yandex'):
        logging.info('Generating GRPC Internal Api spec - skipped (directory already exist)')
        return

    logging.info('Generating GRPC Internal Api spec')
    ya_cmd = '../../../ya'
    source_dir = '../../bitbucket'
    dirs = [
        ('/common-api/yandex/cloud/api', 'yandex/cloud/api/'),
        ('/common-api/yandex/cloud/api/tools', 'yandex/cloud/api/tools/'),
        ('/private-api/yandex/cloud/priv/billing/v1', 'yandex/cloud/priv/billing/v1/'),
        ('/private-api/yandex/cloud/priv/compute/v1', 'yandex/cloud/priv/compute/v1/'),
        ('/private-api/yandex/cloud/priv/microcosm/instancegroup/v1', 'yandex/cloud/priv/microcosm/instancegroup/v1/'),
        ('/private-api/yandex/cloud/priv/access', 'yandex/cloud/priv/access/'),
        ('/private-api/yandex/cloud/priv/operation', 'yandex/cloud/priv/operation/'),
        ('/private-api/yandex/cloud/priv/reference', 'yandex/cloud/priv/reference/'),
        ('/private-api/yandex/cloud/priv/mdb/v1', 'yandex/cloud/priv/mdb/v1/'),
        ('/private-api/yandex/cloud/priv/mdb/kafka/v1', 'yandex/cloud/priv/mdb/kafka/v1/'),
        ('/private-api/yandex/cloud/priv/mdb/sqlserver/v1', 'yandex/cloud/priv/mdb/sqlserver/v1/'),
        ('/private-api/yandex/cloud/priv/mdb/sqlserver/v1/config', 'yandex/cloud/priv/mdb/sqlserver/v1/config'),
        ('/private-api/yandex/cloud/priv/vpc/v1', 'yandex/cloud/priv/vpc/v1/'),
        ('/private-api/yandex/cloud/priv', 'yandex/cloud/priv/'),
    ]
    env = get_env_without_python_entry_point()
    for src, dst in dirs:
        subprocess.check_call(shlex.split(f'mkdir -p {dst}'), env=env)
        subprocess.check_call(shlex.split(f'{ya_cmd} make --checkout --add-result=py {source_dir}{src}'), env=env)
        subprocess.check_call(shlex.split(f'rsync -L -r {source_dir}{src}/ {dst}'), env=env)


if __name__ == '__main__':
    main()

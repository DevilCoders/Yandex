import os
import sys
from io import BytesIO

from pkg_resources import parse_version

import boto3
import botocore.config
from cloud.mdb.cli.dbaas.internal import vault
from cloud.mdb.cli.dbaas.internal.cache import cached
from cloud.mdb.cli.dbaas.internal.config import DBAAS_TOOL_ROOT_PATH


def get_version():
    """
    Return version of mdb-scripts.
    """
    tool_dir = os.path.dirname(sys.executable)
    with open(os.path.join(tool_dir, 'version.txt'), 'r') as file:
        return file.read().strip()


def is_dev_version():
    """
    Return `True` if it's used development version.
    """
    return 'dev' in get_version()


def check_version():
    """
    Check that the current version is up-to-date.
    """
    if is_dev_version():
        return

    try:
        current_version_str = get_version()
        current_version = parse_version(current_version_str)
        latest_version_str = get_latest_version()
        latest_version = parse_version(latest_version_str)
        if latest_version > current_version:
            message = (
                f'Warning: There is a new version "{latest_version_str}" available. Current version: "{current_version_str}".'
                ' Please re-run "install.sh" to update the tool to up-to-date version.\n'
                'The list of changes can be reviewed by executing'
                f' "arc log r{_revision(current_version_str)}..r{_revision(latest_version_str)} $(arc root)/cloud/mdb/cli/dbaas".\n'
            )
            print(message, file=sys.stderr)
    except Exception as e:
        print(f'Warning: {e}', file=sys.stderr)


@cached(file=os.path.join(DBAAS_TOOL_ROOT_PATH, 'cache/latest_version'), ttl=24 * 3600)
def get_latest_version():
    """
    Return the latest available version.
    """
    credentials = vault.get_secret('sec-01efpjrrbw9jc6vhf7f2qra6en')

    session = boto3.session.Session(
        aws_access_key_id=credentials['access_key'], aws_secret_access_key=credentials['secret_key']
    )

    client = session.client(
        service_name='s3',
        endpoint_url='https://storage.yandexcloud.net',
        config=botocore.config.Config(
            s3={
                'region_name': 'ru-central1',
            }
        ),
    )

    with BytesIO() as fileobj:
        client.download_fileobj('mdb-scripts', 'current_version', fileobj)
        return fileobj.getvalue().decode().strip()


def _revision(version):
    return version.rsplit('.', 1).pop()

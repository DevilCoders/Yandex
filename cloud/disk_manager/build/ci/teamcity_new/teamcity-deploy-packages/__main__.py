import argparse
import json

from cloud.disk_manager.build.ci.teamcity_new import common
from cloud.disk_manager.build.ci.z2 import Z2Client


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--packages-file',
        type=str,
        help='path to packages.json',
        required=True
    )
    args = parser.parse_args()

    with open(args.packages_file, 'r') as f:
        packages = json.load(f)

    z2_client = Z2Client(
        endpoint='https://z2-cloud.yandex-team.ru/',
        token=common.z2_token()
    )
    z2_client.edit(common.z2_group_to_edit(), packages)
    z2_client.update_sync(common.z2_group_to_update())


if __name__ == '__main__':
    run()

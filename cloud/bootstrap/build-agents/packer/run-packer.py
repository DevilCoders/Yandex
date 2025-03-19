#!/usr/bin/env python3
import argparse
import requests
import json
import subprocess
import sys
import getpass
import os
import signal

TF_STATE = 'https://s3.mds.yandex.net/yc-bootstrap-lts/terraform/preprod/bootstrap'


def main(args):
    tf_state = requests.get(TF_STATE)
    tf_state_json = tf_state.json()
    folder_id = tf_state_json['outputs']['yc-build-agents-folder-id']['value']
    subnet_id = tf_state_json['outputs']['yc-build-agents-subnet-ids']['value'][args.zone]['id']
    envs = os.environ.copy()
    if not envs.get('COMMIT_REVISION'):
        envs['COMMIT_REVISION'] = str(_get_revision_id())
    envs['SSH_USER'] = args.user
    envs['SSH_BASTION_USER'] = args.bastion_user
    envs['YC_ZONE'] = args.zone
    envs['YC_SUBNET_ID'] = subnet_id
    envs['YC_FOLDER_ID'] = folder_id
    envs['YC_ENDPOINT'] = args.api_endpoint
    if not envs.get('YC_TOKEN'):
        envs['YC_TOKEN'] = _update_yc_token()

    rc = _run_packer(envs, args.packer_bin, args.debug)
    if rc:
        print(rc)


def _update_yc_token():
    print('Update iam token')
    cmd = 'yc --profile=preprod iam create-token'
    process = subprocess.run(cmd.split(' '), stdout=subprocess.PIPE)
    return process.stdout.strip().decode("utf-8")


def _get_out_from_proc(proc):
    while True:
        if proc.returncode:
            print(proc.stdout, proc.stderr)
            return proc.returncode
        output = proc.stdout.readline()
        if output == b'' and proc.poll() is not None:
            break
        if output:
            print(output.strip().decode('utf-8'))


def _run_packer(envs, bin, debug):
    cmd = [bin, 'build']
    if not sys.stdout.isatty():
        cmd.append('-machine-readable')
    if debug:
        cmd.append('-debug')
    cmd.append('build-agent-image.json')
    try:
        proc = subprocess.Popen(
            cmd, env=envs,
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        _get_out_from_proc(proc)
        rc = proc.poll()
    except KeyboardInterrupt:
        print('Packer process was interrupted')
        os.kill(proc.pid, signal.SIGINT)
        _get_out_from_proc(proc)
        proc.stdin.close()
        proc.terminate()
        proc.wait(timeout=120)
        return
    return rc


def _get_revision_id():
    result = subprocess.run('arc log -n 1 --oneline --json'.split(' '), stdout=subprocess.PIPE)
    id = json.loads(result.stdout.strip().decode('utf-8'))[0]['revision']
    return id


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-z', '--zone',
        choices=['ru-central1-a', 'ru-central1-b', 'ru-central1-c'],
        default='ru-central1-a',
        help='zone where build will be running'
    )
    parser.add_argument('-u', '--user', default=getpass.getuser())
    parser.add_argument('-b', '--bastion-user', default=getpass.getuser())
    parser.add_argument('-d', '--debug', default=False, action='store_true')
    parser.add_argument('-e', '--api-endpoint', default='api.cloud-preprod.yandex.net:443')
    parser.add_argument('--packer-bin', default='packer', help='path to custom binary for use in build process')

    args = parser.parse_args()
    main(args)

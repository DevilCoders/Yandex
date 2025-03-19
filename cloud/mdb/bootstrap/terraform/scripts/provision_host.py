#!/usr/bin/env python3

import argparse
import subprocess
import time

WAIT_FOR_RETRY = 15  # sec
RETRY_COUNT = 5

parser = argparse.ArgumentParser(description='Bootstrap MDB DNS.')
parser.add_argument('--token', help='a token for selfdns api')
parser.add_argument('--fqdn', help='a fqdn for this host')
parser.add_argument('--deploy', help='a fqdn for deploy master')

args = parser.parse_args()

# DNS
subprocess.check_call([f'hostname {args.fqdn}'], shell=True)
subprocess.check_call([f'echo {args.fqdn} > /etc/hostname'], shell=True)
subprocess.check_call([f'echo "::1  {args.fqdn} localhost" > /etc/hosts'], shell=True)
subprocess.check_call([f'sed -i \'s/dummy/{args.token}/\' /etc/yandex/selfdns-client/default.conf'], shell=True)
subprocess.check_call([f'sed -i \'s/https:\/\/selfdns-api.yandex.net/https:\/\/selfdns-api.cloud.yandex.net/\' /etc/yandex/selfdns-client/default.conf'], shell=True)
subprocess.check_call([f'sed -i /etc/yandex/selfdns-client/plugins/default -e \'/echo.*IPV4_ADDRESS/s/echo \"/echo >\/dev\/null \"/\''], shell=True)
for index in range(RETRY_COUNT):
    try:
        subprocess.check_call([f'selfdns-client --terminal --debug --service=https://selfdns-api.cloud.yandex.net'], shell=True)
        break
    except subprocess.CalledProcessError:
        print(f'Error on try {index + 1}/{RETRY_COUNT} to start selfdns, wait for {WAIT_FOR_RETRY} seconds.')
        time.sleep(WAIT_FOR_RETRY)

subprocess.check_call([f'rm -f /tmp/.grains_conductor.cache'], shell=True)

# Resize
subprocess.check_call([f'echo ",+," | sfdisk --force -N1 /dev/vda || /bin/true'], shell=True)
subprocess.check_call([f'partprobe /dev/vda'], shell=True)
subprocess.check_call([f'resize2fs /dev/vda1'], shell=True)
subprocess.check_call([f'service bind9 restart'], shell=True)

# Deploy
subprocess.check_call([f'echo "2" > /etc/yandex/mdb-deploy/deploy_version'], shell=True)
subprocess.check_call([f'echo "{args.deploy}" > /etc/yandex/mdb-deploy/mdb_deploy_api_host'], shell=True)

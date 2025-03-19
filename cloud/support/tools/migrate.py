import requests
import os
import json
from os.path import expanduser
import configparser
import sys

VM_URL = 'https://iaas.private-api.cloud.yandex.net/compute/private/v1/instances/'
IAM_TOKEN_URL = 'http://identity.private-api.cloud.yandex.net:4336/v1/tokens'
EXTERNAL_URL = 'https://iaas.private-api.cloud.yandex.net/compute/external/v1/instances/'
home_dir = expanduser('~')
config = configparser.RawConfigParser()
config.read('{}/.rei/rei.cfg'.format(home_dir))

try:
    oauth = config.get('REI_AUTH', 'oAuth_token')
    ca_cert = config.get('CA', 'cert')
    open(ca_cert, 'r')
    os.environ['REQUESTS_CA_BUNDLE'] = ca_cert
except (FileNotFoundError, ValueError, configparser.NoSectionError):
    print('\nCorrupted config or no config file present.\nInitialization...')


class Compute:
    def __init__(self):
        r = requests.post(IAM_TOKEN_URL, json={'oauthToken': oauth})
        iam_token = r.json()['iamToken']
        expire = r.json()['expiresAt']
        self.iam_token = iam_token
        #print(self.iam_token)

    def get_node(self,instance_id):
        r = requests.get(VM_URL + instance_id)
        res = json.loads(r.text)
        compute_node = res.get('compute_node')
        print(compute_node)
        return compute_node

    def migrate_offline(self,instance_id):
        node=self.get_node(instance_id)
        headers = {'X-YaCloud-SubjectToken': self.iam_token}
        r = requests.post(EXTERNAL_URL+instance_id+'/migrateOffline', json={'compute_node' : node}, headers=headers)
        res = json.loads(r.text)
        if r.status_code != 200:
            print('Error',r.status_code,res)
        else:
            print(r.status_code,' OK! ', instance_id)


def load_list(instance):
    compute=Compute()
    with open(instance,'r') as infile:
        inst_arr = infile.readlines()
        for vm in inst_arr:
            instance_id = vm.strip()
            if instance_id:
                compute.migrate_offline(instance_id)


if __name__=='__main__':
    load_list(sys.argv[1])

import pynetbox
import yaml
import os
import requests
from ncclient import manager
from jinja2 import Environment, FileSystemLoader
from decouple import config

curr_dir = os.path.dirname(os.path.abspath(__file__))
env = Environment(
    loader=FileSystemLoader(curr_dir),
    trim_blocks=True,
    lstrip_blocks=True
)
template = env.get_template('flows.j2')


def get_flows(token):
    # with open('flows.yml') as f:
    #     return yaml.safe_load(f)
    response = requests.get(
        'https://bb.yandex-team.ru/rest/api/1.0/projects/CLOUD/repos/cloud-filters/raw/flows.yml?raw',
        auth=('x-oauth-token', token)
    )
    return yaml.safe_load(response.text)


def configure_rr(netbox_rrs, template, config):
    for rr in netbox_rrs:
        try:
            with manager.connect(
                    host=rr.name,
                    port=22,
                    username='netinfra-rw',
                    key_filename='/home/netinfra-rw/.ssh/id_rsa',
                    hostkey_verify=False,
                    # ssh_config=True,
                    device_params={'name': 'junos'}
            ) as m:

                payload = template.render(config)
                print(rr.name)
                # print(payload)
                # with open('{}.txt'.format(rr.name), 'w') as f:
                #     f.write(payload)

                netconf_reply = m.edit_config(config=payload, target='candidate')
                print(netconf_reply)
                m.commit()
        except OSError:
            print("Couldn't connect to {}".format(rr.name))


def main():
    nb = pynetbox.api('http://netbox.cloud.yandex.net')
    fsrrs = nb.virtualization.virtual_machines.filter(tenant='production', role='fsrr')
    configure_rr(fsrrs, template, get_flows(config('TOKEN')))


if __name__ == '__main__':
    main()

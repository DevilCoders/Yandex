from abbreviation_client import AutoAbbreviationClient, BooleanParameterType, EnumParameterType, PositionalParameterType
from abbreviation_client.Colors import red, yellow, blue, green, gray
from os.path import expanduser
from collections import defaultdict
from json import dumps
import requests
import sys
import re
import os


DC_TYPE = EnumParameterType(["sas", "man", "iva", "myt", "vla"])
POOL_TYPE = EnumParameterType(["alet", "media", "music", "tokk"])
OS_TYPE = EnumParameterType(["xenial", "trusty"])
HDD_TYPE = EnumParameterType(["hdd", "ssd"])
STATUS_TYPE = EnumParameterType(["running", "stopped", "migrating"])


class IceClient(AutoAbbreviationClient):
    client_name = 'The Icecream Client'

    host_line = yellow('{fqdn:<58}  ') \
                + gray('cpu ') + blue('{cpu_used:>2}/{cpu_total:>2}  ') \
                + gray('mem ') + blue('{mem_used:>3}/{mem_total:>3}  ') \
                + gray('dsk ') + blue('{dsk_used:>4}/{dsk_total:>4}  ') \
                + yellow('  {pool}')

    cont_line = '  {fqdn:<53}  ' \
                + '{status}  ' \
                + gray('cpu ') + blue('{cpu:>5}  ') \
                + gray('mem ') + blue('{mem:>7}  ') \
                + gray('dsk ') + blue('{dsk:>9}  ') \
                + yellow('  {project}')

    cont_line_ext = yellow('{fqdn:<55}  ') \
                    + '{status}  ' \
                    + gray('cpu ') + blue('{cpu:>2}  ') \
                    + gray('mem ') + blue('{mem:>3}  ') \
                    + gray('dsk ') + blue('{dsk:>4}  ') \
                    + yellow('{project:>6}') \
                    + '    {dom0} ({pool})'

    total_line = blue('total') + ' ' * 55 \
                 + blue('{cpu:>6}  ') \
                 + blue('{mem:>7}  ') \
                 + blue('{dsk:>8}  ')

    def __init__(self):
        super().__init__()

        self.API = 'https://i.yandex-team.ru/v1'
        self.CONDUCTOR_API = 'https://c.yandex-team.ru/api'

        # get token
        try:
            self.token = open(expanduser('~/.ice')).read().strip()
        except FileNotFoundError:
            print('Please generate an OAuth token and put it in ~/.ice:')
            print('https://oauth.yandex-team.ru/authorize?'
                  'response_type=token&client_id=a2998d2d2deb405eac094e586ef360e2')
            sys.exit(-1)

        # get pem path
        pkg_dir, _ = os.path.split(__file__)
        self.pem_path = os.path.join(pkg_dir, "data", "Yandex.pem")

        self.run()

    def _base_fetch(self, api, url, verify_path=None, headers={}, method='GET', data=None, timeout=10):
        """ Generic fetch for any API """
        try:
            req = requests.request(method,
                                   '%s/%s' % (api, url),
                                   data=dumps(data),
                                   headers=headers,
                                   timeout=timeout,
                                   verify=verify_path)
            if req.status_code != 200:
                print(red(req.json()["detail"]))
                sys.exit(-1)
            return req.json()
        except requests.exceptions.RequestException as x:
            print(red(str(x)))
            sys.exit(-1)

    def _fetch(self, url, method='GET', data=None, timeout=10):
        """ Fetch with icecream API """
        headers={'Authorization': 'OAuth %s' % self.token,
                 'Content-type': 'application/json'}
        return self._base_fetch(self.API, url, self.pem_path, headers, method, data, timeout)

    def _conductor(self, url):
        return self._base_fetch(self.CONDUCTOR_API, url + '?format=json')

    @staticmethod
    def _status_with_color(status):
        status_map = {
            'running': green('►'),
            'stopped': red('■'),
            'migrating': yellow('♞'),
        }
        if status not in status_map:
            return yellow(status)
        return status_map[status]

    def physical_list(self,
                      pool: POOL_TYPE =None,
                      project: "name" =None,
                      dc: DC_TYPE =None,
                      name: "fqdn" =None,
                      status: STATUS_TYPE =None,
                      disk: HDD_TYPE =None,
                      verbose: BooleanParameterType() =True):

        """ List physical hosts """
        for host in self._fetch('list'):

            # filter out non-required hosts
            if pool and host['pool'] != pool:
                continue
            if dc and host['datacenter'] != dc:
                continue
            if name and not re.search(name, host['fqdn']):
                continue
            if disk and host['diskType'] != disk:
                continue

            # print the remaining ones
            if not verbose:
                print(host['fqdn'])
                continue

            # print verbose
            cpu = 0
            disk_size = 0
            ram = 0
            containers = []
            for cont in host['containers']:
                cpu += cont['cpu']
                disk_size += cont['diskSize']
                ram += cont['ram']
                if project and cont['project'] != project:
                    continue
                if status and cont['status'].lower() != status:
                    continue
                containers.append(cont)

            # skip host when containers filtered out
            if (project or status) and not containers:
                continue

            host_info = self.host_line.format(fqdn=host['fqdn'],
                                              cpu_used=cpu,
                                              cpu_total=host['cpu'],
                                              mem_used=ram // 1073741824,
                                              mem_total=host['ram'] // 1073741824,
                                              dsk_used=disk_size // 1073741824,
                                              dsk_total=host['diskSize'] // 1073741824,
                                              pool=host['pool'],
                                              )
            print(host_info)

            for cont in containers:
                cont_info = self.cont_line.format(fqdn=cont['fqdn'],
                                                  cpu=cont['cpu'] if 'cpu' in cont else '',
                                                  mem=cont['ram'] // 1073741824,
                                                  dsk=cont['diskSize'] // 1073741824,
                                                  project=cont['project'],
                                                  status=self._status_with_color(cont['status'].lower()),
                                                  )
                print(cont_info)
            print()

    def container_create(self,
                         fqdn: PositionalParameterType(),
                         dc: DC_TYPE =None,
                         project: "name" =None,
                         os: OS_TYPE ="trusty",
                         profile: "name" =None,
                         cpu: "number" =1,
                         ram: "GB" =4,
                         hdd: HDD_TYPE ="hdd",
                         disk_size: "GB" =50
                         ):
        """ Create container """
        data = {
            "fqdn": fqdn,
            "project": project,
            "image": os,
            "profiles": [profile],
            "dc": dc,
            "cpu": int(cpu),
            "ram": int(ram) * 1073741824,
            "diskType": hdd,
            "diskSize": int(disk_size) * 1073741824 }
        ret = self._fetch("container", method="POST", data=data, timeout=600)
        print(green(ret["msg"] if ret["msg"] is not None else "success"))

    def container_destroy(self, fqdn: PositionalParameterType()):
        """ Destroy container """
        ret = self._fetch("container/{}".format(fqdn), method="DELETE", timeout=600)
        print(green(ret["msg"] if ret["msg"] is not None else "success"))

    def container_list(self,
                       pool: POOL_TYPE =None,
                       project: "name" =None,
                       dc: DC_TYPE =None,
                       name: "fqdn" =None,
                       status: STATUS_TYPE =None,
                       verbose: BooleanParameterType() =True):
        """ List containers """
        data = self._fetch('list')
        for host in data:

            # filter out hosts
            if dc and host['datacenter'] != dc:
                continue

            if pool and host['pool'] != pool:
                continue

            # now the containers
            for cont in host['containers']:

                # filter them out
                if project and cont['project'] != project:
                    continue
                if name and not re.search(name, cont['fqdn']):
                    continue
                if status and cont['status'].lower() != status:
                    continue

                # dump containers
                if not verbose:
                    print(cont['fqdn'])
                    continue

                # now the verbose part
                cont_info = self.cont_line_ext.format(
                    fqdn=cont['fqdn'],
                    cpu=cont['cpu'] if 'cpu' in cont else '',
                    mem=cont['ram'] // 1073741824,
                    dsk=cont['diskSize'] // 1073741824,
                    project=cont['project'],
                    pool=host['pool'],
                    dom0=host['fqdn'],
                    status=self._status_with_color(cont['status'].lower()),
                )
                print(cont_info)

    def container_modify(self,
                         fqdn: PositionalParameterType(),
                         cpu: "N" =None,
                         mem: "GB" =None,
                         disk: "GB" =None,
                         ):
        """ Update container resources """
        if not cpu and not mem and not disk:
            print(red("Need at least one change"))
            return

        container = None
        container_list = self._fetch('list')
        for host in container_list:
            for cont in host['containers']:
                if cont['fqdn'] == fqdn:
                    container = cont
                    break

        if not container:
            print(red("Cannot find container {}".format(fqdn)))
            return

        data = {
            "resize": {
                "diskSize": int(disk) * 1073741824 if disk else container['diskSize'],
                "ram": int(mem) * 1073741824 if mem else container['ram'],
                "cpu": int(cpu) if cpu else container['cpu'],
            }
        }

        ret = self._fetch("container/{}".format(fqdn), method="PUT", data=data, timeout=600)
        print(green(ret["msg"] if ret["msg"] is not None else "success"))

    def container_migrate(self,
                          fqdn: PositionalParameterType(),
                          to: "fqdn" =None,
                          ):
        """ Migrate a container to the other physical host """

        print('Starting migration (sync, slooow)...')
        self._fetch('container/%s' % fqdn,
                    method='PUT',
                    data={'target_fqdn': to or '', 'migrate': 'move'},
                    timeout=600)
        print('Done!')

    def container_start(self, fqdn: PositionalParameterType()):
        """ Start a container """
        print('Starting %s' % fqdn)
        self._fetch('container/%s' % fqdn,
                    method='PUT',
                    data={'status': 'start'},
                    timeout=600)
        print('Done!')

    def container_stop(self, fqdn: PositionalParameterType()):
        """ Stop a container """
        print('Stopping %s' % fqdn)
        self._fetch('container/%s' % fqdn,
                    method='PUT',
                    data={'status': 'stop'},
                    timeout=600)
        print('Done!')

    def resource_conductor_group(self, name: PositionalParameterType()):
        """ Calculate resource usage for the containers in the conductor group """
        names = set([x['fqdn'] for x in self._conductor('groups2hosts/' + name)])

        cpu_total = 0
        mem_total = 0
        dsk_total = 0
        data = self._fetch('list')
        for host in data:
            for cont in host['containers']:
                if cont['fqdn'] not in names:
                    continue

                cont_info = self.cont_line_ext.format(
                    fqdn=cont['fqdn'],
                    cpu=cont['cpu'] if 'cpu' in cont else '',
                    mem=cont['ram'] // 1073741824,
                    dsk=cont['diskSize'] // 1073741824,
                    project=cont['project'],
                    pool=host['pool'],
                    dom0=host['fqdn'],
                    status=self._status_with_color(cont['status'].lower()),
                )
                print(cont_info)

                if 'cpu' in cont:
                    cpu_total += cont['cpu']
                mem_total += cont['ram']
                dsk_total += cont['diskSize']

        total_info = self.total_line.format(
            cpu=cpu_total,
            mem=mem_total // 1073741824,
            dsk=dsk_total // 1073741824,
        )
        print()
        print(total_info)

    def info(self):
        """ Display various icecream info """
        data = self._fetch('info')
        for item, elts in data.items():
            print(yellow(item))
            if item == 'profiles':
                elts = sorted(elts)
                profile_map = defaultdict(list)
                for elt in elts:
                    profile_map[elt.split('-')[0]].append(elt)
                for line in profile_map:
                    print('  ' + ' '.join(profile_map[line]))
            else:
                print('  ' + ' '.join(sorted(elts)))
            print()

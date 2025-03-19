#!/usr/bin/python3
"""
Main BUHLO class. Listen http requests and deploy hosts.
BUHLO: Balanced Unified Host Loading Operation
"""
import socket
import subprocess
import logging
from time import sleep
import concurrent.futures
import requests
from flask import Flask, request
from media_dolivka.monitoring import Monitor
from os.path import exists
from json import loads as load_from_json
from json import dumps as load_to_json


def ping_host(host):
    """
    Try connect host via ssh and check for ability to apply deploy playbook
    """
    log = logging.getLogger("ping_host")
    command = [
        'ansible-playbook',
        '--private-key=/root/.ssh/salt_id',
        '/var/lib/dolivka/ansible/test.yml',
        '-e',
        'host=' + host
    ]
    try:
        msg = "Run check playbook command: {}".format(" ".join([x for x in command]))
        log.debug(msg)
        status = subprocess.check_call(command, stdout=subprocess.DEVNULL)
    except (OSError, subprocess.CalledProcessError):
        log.info('Host %s status: offline', host)
        return False

    if status == 0:
        log.info('Host %s status: online', host)
        ret = True
    else:
        log.info('Host %s status: offline', host)
        ret = False

    return ret


def rundeploy(host):
    """
    wait host appears online
    timw to wait in sec.
    TODO: place this options to config
    """
    log = logging.getLogger("rundeploy")

    wait_time = 1200

    host_online = False
    while wait_time > 0 and not host_online:
        host_online = ping_host(host)
        if not host_online:
            log.info("Host %s offline.", host)
            sleep(30)
            wait_time -= 30

    log.info("Host %s comes online. Start deploy.", host)
    try:
        subprocess.call(['/var/lib/dolivka/ansible/rundeploy.sh', host])
    except (OSError, subprocess.CalledProcessError) as exc:
        log.error("rundeploy failed %s", getattr(exc, "message", str(exc)))
        return False

    return True


def ishostexists(host):
    """Check host existence"""
    log = logging.getLogger("ishostexists")

    try:
        socket.getaddrinfo(host, None, 0, 0, socket.AI_ADDRCONFIG)
    except socket.gaierror as fail:
        log.warning("getaddinfo failed for %s: %s", host, fail)
        return False

    return True


class Buhlo:  # pylint: disable=old-style-class
    """
    Main BUHLO HTTP API class
    TODO: manage config/secrets-default.yaml by salt-secure;
    fix playbook/tasks/setenv: remove set group;
    place project_group into config(must be managed by salt too)
    """

    def __init__(self, conf):
        self.log = logging.getLogger(self.__class__.__name__)
        self.conf = conf
        self.monitor = Monitor(self.conf.monitoring)
        self.executor = concurrent.futures.ThreadPoolExecutor(max_workers=2)
        self.log.debug(conf.toDict().__str__())

        self.app = Flask(__name__)
        self.port = int(conf.port)
        self.addr = conf.address
        self.app.add_url_rule('/ping', 'ping', self.ping)
        self.app.add_url_rule('/mon', 'mon', self.mon)
        self.app.add_url_rule('/deploy/<host>', 'deploy', self.deploy)

        self.app.register_error_handler(404, self.not_found)
        self.app.register_error_handler(500, self.internal_error)
        self.app.after_request(self.response_handle)

        if not conf.project_group:
            self.project_group = 'media-dom0-lxd'
        else:
            self.project_group = conf.project_group

        if not conf.secrets.conductor_oauth:
            self.log.warn('Failed to get config value dolivka.secrets.conductor_oauth.')
            self.conductor_oauth = None
        else:
            self.conductor_oauth = conf.secrets.conductor_oauth

    def getgroupid(self, group):
        """
        returns group id(on error returns default group media-dom0-lxd)
        :param group: group name
        :return: group str
        """
        url = 'https://c.yandex-team.ru/api/v1/groups/{}'.format(group)
        headers = {'Authorization': 'OAuth {}'.format(self.conductor_oauth),
                   'Accept': 'application/json',
                   'Content-Type': 'application/json'
                   }
        g_id = '24992'
        try:
            data = requests.get(url, timeout=1, headers=headers)
            if data.status_code != 200:
                self.log.error('Failed to get group id for %s. Conductor returns %s: %s',
                               group, data.status_code, data.content.decode('utf-8'))
                self.log.debug("Failed request: url: %s, data: %s with headers %s",
                               url, headers)
            else:
                g_id = load_from_json(data.content.decode('utf-8'))['id']
        except requests.exceptions.RequestException as exc:
            self.log.error('Cant get group data {}. Error: {}'.format(group, exc.strerror))

        return g_id

    def host2groups(self, host):
        """
        calls conductor api handle host2groups: get hosts groups
        :param host: fqdn
        :return: groups list or None
        """
        url = 'https://c.yandex-team.ru/api/hosts2groups/{}'.format(host)
        headers = {'Authorization': 'OAuth {}'.format(self.conductor_oauth),
                   'Accept': 'application/json',
                   'Content-Type': 'application/json'
                   }
        groups = None
        try:
            data = requests.get(url, timeout=1, headers=headers)
            if data.status_code != 200:
                self.log.error('Failed to get group id for %s. Conductor returns %s: %s',
                               host, data.status_code, data.content.decode('utf-8'))
                self.log.debug("Failed request: url: %s, data: %s with headers %s",
                               url, headers)
            else:
                groups = data.content.decode('utf-8').split()
                self.log.debug("Host %s belongs theese groups: %s", host, groups)
        except requests.exceptions.RequestException as exc:
            self.log.error("Filed to get host groups. Error: %s", exc.strerror)

        return groups

    def setgroup(self, host, group):
        """
        set conductor group according dom0 project
        :return: None
        """
        host_groups = self.host2groups(host)
        if group in host_groups:
            self.log.info("Host %s is already in group %s.", host, group)
            return True

        group_id = self.getgroupid(group)
        req_data = load_to_json({"host": {"group": {"id": group_id}}})
        req_headers = {'Authorization': 'OAuth {}'.format(self.conductor_oauth),
                       'Accept': 'application/json',
                       'Content-Type': 'application/json'
                       }
        url = 'https://c.yandex-team.ru/api/v1/hosts/{}'.format(host)

        try:
            data = requests.put(url, data=req_data, headers=req_headers, timeout=1)
            if data.status_code != 200:
                self.log.error('Failed to set group %s. Conductor returns %s: %s',
                               group, data.status_code, data.content.decode('utf-8'))
                self.log.debug("Failed request: url: %s, data: %s with headers %s",
                               url, req_data, req_headers)
                return False
        except requests.exceptions.RequestException:
            self.log.error('Cant set group {} for host {}'.format(group, host))

        return True

    def ping(self):
        """App check ping"""
        status = self.monitor.get_status()
        status = int(status)
        if status > self.monitor.crit_thr:
            msg = u"Ansible test failed {}/{}. Status is CRIT.".format(
                status,
                self.monitor.crit_thr
            )
            self.log.info(msg)
            resp = (u"2;{}".format(msg), 500)
        else:
            msg = u"Ansible test failed {}/{}. Status still OK.".format(
                status,
                self.monitor.crit_thr
            )
            self.log.info(msg)
            resp = (u"0;{}".format(msg), 200)
        return resp

    def mon(self):
        """
        Monitoring
        this is not very fast and must be used not often; 5-10 seconds is ok
        """

        # at first try simple way...
        if exists('/etc/dom0hostname'):
            with open('/etc/dom0hostname') as f:
                host = f.readlines()[0]
        else:
            try:
                resp = requests.get(
                    "https://c.yandex-team.ru/api-cached/groups2hosts/media-dom0-lxd")
                host = list(resp.content.split())[-1].decode('utf-8')
            except requests.exceptions.RequestException as exc:
                msg = u"2;Can't get data from conductor {}".format(exc)
                self.log.info(msg)
                self.monitor.inc_state()
                return msg, 500

        msg = "Selected host for ping check: {}".format(host)
        self.log.debug(msg)
        ret = ping_host(host)

        if ret:
            self.log.info('Ansible test passed, set statius to OK')
            self.monitor.set_status("0")
            resp = (u"0;OK\n", 200)
        else:
            status = int(self.monitor.get_status())
            if status > self.monitor.crit_thr:
                msg = u"Ansible test failed {}/{}. Status is CRIT.".format(
                    status, self.monitor.crit_thr
                )
                self.log.info(msg)
                resp = (u"2;{}".format(msg), 500)
            else:
                status += 1
                msg = u"Ansible test failed {}/{}. Status still OK.".format(
                    status, self.monitor.crit_thr
                )
                self.log.info(msg)
                self.monitor.inc_state()
                resp = (u"0;{}".format(msg), 200)

        return resp

    def deploy(self, host):
        """
        run host deploy with ansible
        :param host: hostname
        """
        group = request.args.get('group')
        if not group:
            group = self.project_group
            msg = 'Using default project group: {}'.format(self.project_group)
        else:
            msg = 'Project group {} was overrided in get request: {}'.format(
                self.project_group, group
            )

        self.log.debug(msg)

        if not ishostexists(host):
            msg = 'Host {} has no DNS entry'.format(host)
            self.log.error(msg)
            return msg, 400

        if self.conductor_oauth:
            result = self.setgroup(host, group)
            if result == False:
                return 'Failed to start deploy.', 500

        self.log.info('Starting deploy for host ' + host)

        result = self.executor.submit(rundeploy, host)
        if result:
            self.log.info('Deploy started.')
            return 'OK', 200

        self.log.error('Deploy start failed')
        return 'Failed to run deploy script', 500

    def not_found(self):  # pylint: disable=no-self-use
        """Not found handler"""
        return 'Host not found in request. please use /deploy/_host_fqdn', 404

    def internal_error(self, error):  # pylint: disable=no-self-use
        """global error handler"""
        return '2;Something went wrong... {}'.format(error), 500

    def response_handle(self, response):
        """helper for log response"""
        msg = " {} {} {} {} {}".format(
            request.remote_addr,
            request.method,
            request.scheme,
            request.full_path,
            response.status_code)
        self.log.info(msg)
        return response

    def run(self):
        """
        class main
        """
        self.log.info('Starting listening for HTTP')
        self.app.run(self.addr, self.port)
        self.log.info('Stopped listening for HTTP')

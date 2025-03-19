import requests
import re
import socket
import netaddr
from subprocess import Popen, CalledProcessError, PIPE


class bcolors:
    PURRPLE = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNYELLOW = '\033[93m'
    FAILRED = '\033[91m'
    DEFAULT = '\033[0m'
    BOLDWHITE = '\033[1m'
    UNDERLINE = '\033[4m'


class utils():

    def __init__(self, verbose):
        self.verbose = verbose

    def group_to_macro(self, group=str()):
        if re.match('^%[a-z0-9\._\-]+', group):
            return "_C_" + group.replace('%', '').replace('-', '_').upper() + "_"
        elif re.match('^_[a-zA-Z_]+', group):
            return group.upper()
        else:
            return None

    def group2hosts(self, group):
        group = group.replace('%', '')
        url = "https://c.yandex-team.ru/api/groups2hosts/" + group
        try:
            data = requests.get(url)
            if data.status_code != 200:
                self.msg("Failed to get hosts list for " + group + ". http status code " + data.status_code, bcolors.FAILRED)
                return None
        except:
            self.msg("Can't get data for group " + group, bcolors.FAILRED)
            return None

        #print data.content

        return data.content.split()


    def msg(self, message, color=bcolors.DEFAULT):
        # print messages
        if self.verbose:
            print(color + message + bcolors.DEFAULT)

        return True

    def net_to_mhost(self, net):
        # we should know what we get: ipv6 or ipv4
        if re.match("([\d]+\.[\d]+\.[\d]+\.[\d]+/[\d]+)", net):
            maddr = netaddr.IPNetwork(net).network.__str__() + "/" + netaddr.IPNetwork(net).netmask.__str__()
        # elif re.match("((?:[0-9a-fA-F]*:)+:/[\d]+)", net):
        elif self.is_valid_cidr(net):
            if not re.match("([\d]+\.[\d]+\.[\d]+\.[\d]+)", net):
                # print net
                tmp = net.split(":")
                if re.match("/[0-9]+", tmp[-1]):
                    # print tmp
                    tmp[-2] = "%"
                    tmp.pop(-1)
                else:
                    # print tmp
                    tmp[-1] = "%"

                maddr = ":".join(tmp)
        # find one ipv6 addr
        elif re.match("((?:[0-9a-fA-F]*:)+)", net):
            maddr = net
        # find one ipv4 addr
        elif re.match("([\d]+\.[\d]+\.[\d]+\.[\d]+)", net):
            maddr = net
        else:
            return None

        return maddr

    def resolve_host(self, host):
        try:
            info = socket.getaddrinfo(host, None, 0, 0, socket.AI_ADDRCONFIG)
        except socket.gaierror as fail:
            self.msg("Failed to get addrinfo for " + host + " " + fail.__str__(), bcolors.FAILRED)
            return None
        a = []
        for i in info:
            a.append(i[4][0])
        return a

    def is_valid_cidr(self, address):
        """Check if address is valid

        The provided address can be a IPv6 or a IPv4
        CIDR address.
        """
        try:
            # Validate the correct CIDR Address
            netaddr.IPNetwork(address)
        except netaddr.core.AddrFormatError:
            return False
        except UnboundLocalError:
            # NOTE(MotoKen): work around bug in netaddr 0.7.5 (see detail in
            # https://github.com/drkjam/netaddr/issues/2)
            return False

        # Prior validation partially verify /xx part
        # Verify it here
        ip_segment = address.split('/')

        if (len(ip_segment) <= 1 or ip_segment[1] == ''):
            return False
        return True

    def get_grants_repo(self):
        try:
            fqdn = socket.gethostname()
        except socket.error as fail:
            print "Can't get hostname. " + fail.__str__()
            return None

        url = "http://c.yandex-team.ru/api/generator/mysql_secure_repository?fqdn=" + fqdn
        try:
            fname = requests.get(url)
            if fname.status_code != 200:
                self.msg("Failed to get grants filename: http status code " + fname.status_code, bcolors.FAILRED)
                return None
        except requests.RequestException as fail:
            self.msg(fail.__str__(), bcolors.FAILRED)
            return None

        return fname.content.replace("\"", "")

    def get_grants_filename(self):
        try:
            fqdn = socket.gethostname()
        except socket.error as fail:
            self.msg("Can't get hostname. " + fail.__str__(), bcolors.FAILRED)
            return None

        url = "http://c.yandex-team.ru/api/generator/mysql_grants_file?fqdn=" + fqdn
        try:
            fname = requests.get(url)
            if fname.status_code != 200:
                self.msg("Failed to get grants filename: http status code " + fname.status_code, bcolors.FAILRED)
                return None
        except requests.RequestException as fail:
            self.msg(fail.__str__(), bcolors.FAILRED)
            return None

        return fname.content.replace("\"", "")

    def expand_macros(self, macro):
        if macro == None:
            return None

        url = "http://ro.admin.yandex-team.ru/api/firewall/expand_macro.sbml?macro=" + macro
        hosts = re.compile("[a-zA-Z0-9\.?\-?]{5,}\.?[a-z]+")
        ips = []

        # if got just hostname try to resolv
        if hosts.match(macro):
            # print resolve_host(macro)
            return self.resolve_host(macro)

        try:
            data = requests.get(url)
        except requests.ConnectionError as fail:
            self.msg("API requect " + url + " failed with " + fail.__str__(), bcolors.FAILRED)

        if data.status_code == 200:
            h = hosts.findall(data.content)

            # find ip
            tmp = data.content.split()
            ipsall = []
            for w in tmp:
                if not hosts.match(w):
                    if self.is_valid_cidr(w):
                        ipsall.append(w)

            haddr = []
            if h is not None:
                for host in h:
                    addr = self.resolve_host(host)
                    if addr != None:
                        haddr.extend(addr)

            ips.extend(haddr)
            ips.extend(ipsall)
            # check if the macro is empty
            if len(ips) < 1:
                return None

        else:
            self.msg('Failed to expand macro ' + macro + ' reason: golem api response is ' + data.status_code.__str__() + ' content is ' + data.content.__str__(), bcolors.FAILRED)
            self.msg('Hosts which has no IP addresses will be dropped from mysql! May be groups or macroses changed.', bcolors.FAILRED)
            return None

        return ips

    def run(self, cmd):
        try:
            proc = Popen(cmd, stderr=PIPE, stdout=PIPE)
            # print proc.stdout.readline()
            return proc
        except CalledProcessError as e:
            self.msg('Failed to run '+cmd, bcolors.FAILRED)

    def savedata(self, fname, data):
        with open(fname, 'w') as conf:
            conf.write(data.getvalue())
        conf.close()

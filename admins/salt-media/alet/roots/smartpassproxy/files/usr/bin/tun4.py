#!/usr/bin/env python
import os
import sys
import argparse
import logging


try:
    sys.path.append("/usr/bin")
    import ip_tunnel
except ImportError:
    print("ip_tunnel not present. It is in yandex-search-ip-tunnel deb package")
    sys.exit(0)


IP_POOLS = {
    "vla": "77.88.4.96/27",
    "sas": "37.140.189.0/27",
    "iva": "213.180.210.64/28",
    "myt": "95.108.248.160/27"
}
RT_TOKEN_ENVKEY = "RT_TOKEN"
RT_TOKEN_FILE = "/iptunnel-key"
RT_TOKEN_URL = "http://oauth.yandex-team.ru/authorize?" \
               "response_type=token&client_id=950f303bdd4446f3870c8d4156a1c1c9"

logging.basicConfig(format="%(asctime)s %(levelname)5s %(funcName)s:%(lineno)-4s %(message)s", level=logging.INFO)


class Tunneler(object):

    def __init__(self, dc=None):
        self._dc = dc

    @property
    def dc(self):
        if self._dc:
            return self._dc
        try:
            return os.environ["DEPLOY_NODE_DC"]
        except KeyError:
            raise RuntimeError("No DEPLOY_NODE_DC env variable, no dc passed via args")

    @staticmethod
    def _get_rt_token_from_env():
        return os.environ[RT_TOKEN_ENVKEY]

    @staticmethod
    def _get_rt_token_from_file():
        with open(RT_TOKEN_FILE) as rttfd:
            return rttfd.read().strip()

    @property
    def rt_token(self):
        try:
            return self._get_rt_token_from_env()
        except KeyError:
            try:
                return self._get_rt_token_from_file()
            except IOError:
                raise RuntimeError("No %s env variable, no %s file. You can get token here: %s" % (RT_TOKEN_ENVKEY, RT_TOKEN_FILE, RT_TOKEN_URL))

    def initiate_tunnel(self):
        ip_tunnel.RT_TOKEN = "OAuth {}".format(self.rt_token)
        ip_tunnel.IPv4_NETS = {'ext': IP_POOLS}
        sys.argv = ['', 'start', '--geo', self.dc]
        ip_tunnel.main()

    def run(self):
        self.initiate_tunnel()


def parse_args():
    parser = argparse.ArgumentParser("Tunnel digging machine")
    parser.add_argument("-g", "--geo", default=None, action='store', choices=IP_POOLS.keys(), help="Force specific dc")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    Tunneler(dc=args.geo).run()

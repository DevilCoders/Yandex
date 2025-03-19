#!/usr/bin/env python2
import argparse
import logging as log
import os
import xml.etree.ElementTree
from subprocess import check_output

import requests

requests.packages.urllib3.disable_warnings()
log.basicConfig(
    format="%(asctime)s %(levelname)5s: %(lineno)4s#%(funcName)-12s %(message)s",
    level=log.INFO,
)

PATH = "trunk/arcadia/admins/combaine/configs"
SWD = os.environ.get("BSCONFIG_IDIR", os.environ.get("SWD", "."))

ARCANUM_HTTP = "https://a.yandex-team.ru"
API_URL = "{}/api/tree/history/{}"
ARCANUM_SVN = "svn+ssh://{}arcadia-ro.yandex.ru"
SVN_URL = "{}/arc/{}"


class SVNPoller():
    def __init__(self, args):
        self.args = args
        if not self.args.id_rsa:
            self.args.id_rsa = os.path.join(SWD + "/ssh/id_rsa")
            log.debug("Use default id_rsa location %s", self.args.id_rsa)

        self.target = self.args.target or os.path.basename(self.args.path)

    def get_svn_revision(self):
        revision = 4958444  # fallback revision
        if not os.path.exists(self.target):
            return revision

        cmd = ["svn", "info", "--xml", self.target]
        result = check_output(cmd)

        root = xml.etree.ElementTree.fromstring(result)
        entry_attr = root.find('entry').attrib
        revision = entry_attr['revision']

        return int(revision)

    def check_new_revision(self):
        token = os.environ["ARCANUM_TOKEN"]
        url = API_URL.format(ARCANUM_HTTP, self.args.path)
        current_svn_revision = self.get_svn_revision()
        log.info("Query url %s", url)
        resp = requests.get(url,
                            params={
                                "limit": 1,
                                "peg": -1,
                                "repo": "arc"
                            },
                            headers={
                                "Authorization": "OAuth {}".format(token),
                                "User-Agent": PATH,
                            },
                            verify=False)
        resp.raise_for_status()
        rev = resp.json()[0]["revision"]
        current_arc_revision = int(rev.strip("r"))
        log.info("Current arc revision=%d", current_arc_revision)
        if current_svn_revision < current_arc_revision:
            log.info("Current svn revision=%d is too old", current_svn_revision)
            return current_arc_revision
        return None

    def svn_pool(self):
        robot = ""
        url = SVN_URL.format(ARCANUM_SVN, self.args.path)

        kwargs = {}
        if not self.args.interactive:
            robot = self.args.robot + "@"
            kwargs["env"] = {"SVN_SSH": "ssh -o StrictHostKeyChecking=no -i " + self.args.id_rsa}
        url = url.format(robot)

        cmd = ["svn", "co", url, self.target]

        log.debug("run svn command %s with kwargs %s", cmd, kwargs)
        out = check_output(cmd, **kwargs)
        log.debug("Svn output %s", repr(out))

    def run(self):
        new_revision = self.check_new_revision()
        if new_revision:
            self.svn_pool()


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-r", "--robot", help="Robot user name", default="robot-media-salt")
    parser.add_argument("--id_rsa",
                        help="Robots ssh private key",
                        default=os.path.join(SWD, "ssh/id_rsa"))
    parser.add_argument("-d", "--debug", help="Enable debug logs", action="store_true")
    parser.add_argument("-t",
                        "--target",
                        help="Target directory for checkout",
                        default="/dev/shm/configs")
    parser.add_argument("-p", "--path", help="svn path to poll and checkout", default=PATH)
    parser.add_argument("--interactive", help="Checkout by current user", action="store_true")
    args = parser.parse_args()

    assert os.environ.get("ARCANUM_TOKEN"), "Expect OAuth token in ARCANUM_TOKEN env var"
    if args.debug:
        log.getLogger().setLevel(log.DEBUG)
        log.debug("args: %s", args)

    SVNPoller(args).run()


if __name__ == '__main__':
    main()
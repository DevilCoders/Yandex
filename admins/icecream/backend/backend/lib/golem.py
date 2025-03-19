#!/usr/bin/env python3
'''golem'''

import logging
import requests
import dotmap


log = logging.getLogger()  # pylint: disable=invalid-name


class Golem():  # pylint: disable=too-few-public-methods
    '''Create host on golem'''
    api = "https://golem.yandex-team.ru/api/manage_host.sbml"
    resps = "yandex_mnt_person_media"
    monitor = "jmon01h.media.yandex.net"

    def create_host(self, container_fqdn):
        """Create golem hosts"""


        payload = {
            "action": "create",
            "hostname": container_fqdn,
            "resps": self.resps,
            "monitor": self.monitor,
        }
        log.info("Send request to golem api: %s, params: %s", self.api, payload)
        resp = None
        try:
            resp = requests.get(self.api, params=payload)
            resp.raise_for_status()
            log.info("Request to golem success: %s", resp.url)
        except Exception as exc:  # pylint: disable=broad-except
            log.error("Request to golem failed: %s", exc)
        resp = resp or dotmap.DotMap()
        resp = dotmap.DotMap({
            "text": resp.text or "",
            "status_code": resp.status_code or 000,
        })

        return resp.text, resp.status_code

if __name__ == '__main__':
    GOLEM = Golem()
    print(GOLEM.create_host("lxd-cloud.music.dev.yandex.net"))

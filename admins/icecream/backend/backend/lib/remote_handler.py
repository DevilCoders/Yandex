#!/usr/bin/env python3
'''send req to remote host'''

import logging
import requests


log = logging.getLogger()  # pylint: disable=invalid-name


class Remote():
    '''Interact with remote lxd api'''

    def __init__(self, fqdn):
        self.fqdn = fqdn
        self.api = "http://{}/{{}}".format(fqdn)

    def get_resources(self):
        '''get resource from remote physical host'''

        url = self.api.format("v1/resources")

        log.debug("Get physical resources from %s", url)
        data = {}
        try:
            resp = requests.get(url)
            resp.raise_for_status()
            data = resp.json()
            log.info("Request success: %s", url)
        except Exception as exc: # pylint: disable=broad-except
            log.error("Request failed, %s: %s", exc, url)
        return data


    def change_lv_size(self, container_name, disk_size):
        '''change lv size on for container'''

        if disk_size == 53687091200:
            log.debug("Use default disk size, pass")
            return disk_size

        headers = {'Content-type': 'text/plain'}
        url = self.api.format("v1/containers/{}/volume/size".format(container_name))

        resp = requests.put(url, json=disk_size, headers=headers)
        resp.raise_for_status()
        lv_size = int(resp.text)
        log.debug("Remote set new lv size: %s", lv_size)
        return lv_size

#!/usr/bin/env python
# coding=utf-8

import os
import sys
import time

from collections import defaultdict

import requests
import urllib3

urllib3.disable_warnings()

QLOUD_INSTALL = "qloud"
QLOUD_EXT_INSTALL = "qloud-ext"
QLOUD_API_URL = 'https://qloud.yandex-team.ru/api'
QLOUD_EXT_API_URL = 'https://qloud-ext.yandex-team.ru/api'

QLOUD_INSTALLATIONS = [QLOUD_INSTALL, QLOUD_EXT_INSTALL]


def get_qloud_tokent():
    token_path = "~/.abc/token"
    abs_token_path = os.path.expanduser(token_path)
    if not os.path.exists(abs_token_path):
        sys.stderr.write("Qloud OAuth key is absent: %s\n" % token_path)
        sys.stderr.write("Read more at https://github.yandex-team.ru/devtools/startrek-python-client#configuring\n")
        exit(1)

    return file(abs_token_path).read().strip()


def _qloud_api_from_installation(instalation):
    if instalation == QLOUD_INSTALL:
        return QLOUD_API_URL
    elif instalation == QLOUD_EXT_INSTALL:
        return QLOUD_EXT_API_URL

    assert False


class QloudException(Exception):
    pass


def change_host_state(installation, host, state, comment):
    url = "{api}/v1/hosts/{host}?state={state}&comment={comment}".format(
        api=_qloud_api_from_installation(installation),
        host=host,
        state=state,
        comment=comment
    )

    req = requests.put(
        url,
        headers={'Authorization': 'OAuth {}'.format(get_qloud_tokent())}
    )

    sys.stderr.write("change_host_state\t%s\t%s\t%s\n" % (host, state, req.status_code))

    if req.status_code != 204:
        raise QloudException("Failed to change state: %s %s" % (host, state))


def change_host_segment(installation, host, segment, comment):
    url = "{api}/v1/hosts/{host}?hardwareSegment={segment}&comment={comment}".format(
        api=_qloud_api_from_installation(installation),
        host=host,
        segment=segment,
        comment=comment
    )

    req = requests.put(
        url,
        headers={'Authorization': 'OAuth {}'.format(get_qloud_tokent())}
    )

    sys.stderr.write("change_host_segment\t%s\t%s\t%s\n" % (host, segment, req.status_code))

    if req.status_code != 204:
        raise QloudException("Failed to change segment: %s %s" % (host, segment))


class QloudHostsData:
    def __init__(self, installation=None):
        self.__installation = installation if installation else QLOUD_EXT_INSTALL

        hosts_data = requests.get(
            "{}/v1/hosts/search".format(_qloud_api_from_installation(installation)),
            headers={'Authorization': 'OAuth {}'.format(get_qloud_tokent())}
        ).json()

        segments_hosts = defaultdict(list)
        hosts_segments = {}

        for host in hosts_data:
            segment = host["segment"]
            name = host["fqdn"]

            segments_hosts[segment].append(name)
            hosts_segments[name] = segment

        self.segments_hosts = segments_hosts
        self.hosts_segments = hosts_segments

    def installation(self):
        return self.__installation

    def segment_hosts(self, segment):
        return self.segments_hosts.get(segment, [])

    def get_host_segment(self, host):
        segment = self.hosts_segments.get(host)
        if not segment:
            return

        return self.__installation + "." + segment


def print_segment_hosts(segment):
    installation = None
    if "." in segment:
        installation, segment = segment.split(".")

    sys.stdout.write("\n".join(sorted(QloudHostsData(installation=installation).segment_hosts(segment))) + "\n")


def print_hosts_segments():
    hosts_data = [QloudHostsData(installation=el) for el in QLOUD_INSTALLATIONS]

    for line in sys.stdin:
        fqnd = line.split()[2]

        segments = [el.get_host_segment(fqnd) for el in hosts_data]
        segments = [el for el in segments if el]
        segment = segments[0] if segments else "unknown"

        sys.stdout.write("%s\t%s\n" % (line.strip("\n"), segment))


def _transfer_host(qloud_host_data, qloud_ext_host_data, host, target_installation, target_segment, comment):
    target_host_data = qloud_ext_host_data if target_installation == "qloud-ext" else qloud_host_data
    assert target_host_data

    current_segment = target_host_data.get_host_segment(host)
    if current_segment == target_segment:
        sys.stderr.write("%s is already at %s segment\n" % (host, current_segment))
        return

    if target_installation == "qloud" and current_segment == "unknown":  # host is still at qloud-ext.reserve
        current_segment = qloud_ext_host_data.get_host_segment(host)

    if current_segment not in ["qloud-ext.reserve", "qloud-ext.gateway", "qloud-ext.reserve-hold"]:
        raise Exception(
            "%s is at %s segment instead of reserve or gateway - should not transfer using this script" % (
                host,
                current_segment
            )
        )

    if target_installation == "qloud":
        raise NotImplementedError()  # "TODO add transfer from qloud-ext to qloud if necessary"

    done = False
    for i in range(10):
        try:
            change_host_state(target_installation, host, "DOWN", comment)
            change_host_segment(target_installation, host, target_segment, comment)
            change_host_state(target_installation, host, "UP", comment)
            done = True
            break
        except QloudException:
            time.sleep(1)
            continue

    if not done:
        raise Exception("failed to transfer %s to %s" % (host, current_segment))


def transfer_hosts(hosts, segment, comment, debug=False):
    assert "." in segment
    target_installation, target_segment = segment.split(".")

    sys.stderr.write("transfer_hosts %s to %s\n" % (",".join(hosts), segment))
    if debug:
        return

    if target_installation == "qloud":
        raise NotImplementedError()  # transfer from qloud-ext to qloud can not be made without changing wall-e project

    qloud_ext_host_data = QloudHostsData(installation="qloud-ext")
    qloud_host_data = QloudHostsData(installation="qloud") if target_installation == "qloud" else None

    for host in hosts:
        _transfer_host(
            qloud_host_data=qloud_host_data,
            qloud_ext_host_data=qloud_ext_host_data,
            host=host,
            target_installation=target_installation,
            target_segment=target_segment,
            comment=comment
        )


def test():
    return


if __name__ == '__main__':
    from optparse import OptionParser

    parser = OptionParser()

    parser.add_option("--hosts", action="store_true")
    parser.add_option("--segment")
    parser.add_option("--test", action="store_true")

    (options, args) = parser.parse_args()

    if options.segment:
        print_segment_hosts(options.segment)
    if options.hosts:
        print_hosts_segments()
    if options.test:
        test()

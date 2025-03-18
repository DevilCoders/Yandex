#!/usr/bin/env python
# coding=utf-8

import requests
import sys

from collections import defaultdict

# remove warning about unverified connection
import urllib3

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)


class WalleScenarioMap:
    def __init__(self):
        base_url = "https://api.wall-e.yandex-team.ru/v1/scenarios?fields=ticket_key,hosts&scenario_type=hosts-transfer,hosts-add&limit=100&offset={offset}"

        offset = 0
        total = 0

        self._inv_scenario = {}
        self._scenario_inv = defaultdict(list)
        self._inv_timestamp = {}

        i = 0

        while True:
            url = base_url.format(offset=offset)
            response = requests.get(url, verify=False)
            if response.status_code != 200:
                raise Exception("source error at %s: %s\n\n" % (url, response.json()))

            data = response.json()

            for scenario_data in data["result"]:
                i += 1

                task = scenario_data["ticket_key"]

                for host in scenario_data["hosts"]:
                    inv = host["inv"]
                    timestamp = host["timestamp"]

                    self._scenario_inv[task].append(inv)

                    cur_timestamp = self._inv_timestamp.get(inv)
                    if cur_timestamp is None or cur_timestamp < timestamp:
                        self._inv_scenario[inv] = task
                        self._inv_timestamp[inv] = timestamp

            new_total = data["total"]
            total = max(total, new_total)

            offset += 100
            if offset > total:
                break

    def inv_to_scenario(self, inv):
        return self._inv_scenario.get(int(inv))

    def scenario_to_inv(self, scenario):
        print len(self._scenario_inv)
        return self._scenario_inv.get(scenario, [])


def find_scenarios():
    map = WalleScenarioMap()

    for inv in sys.stdin.read().split():
        scenario = map.inv_to_scenario(inv)

        sys.stdout.write("%s\t%s\n" % (inv, scenario))


def find_hosts(scenarios):
    map = WalleScenarioMap()

    for scenario in scenarios:
        for inv in map.scenario_to_inv(scenario):
            sys.stdout.write("%s\t%s\n" % (inv, scenario))


def test():
    return


def main():
    from optparse import OptionParser

    parser = OptionParser()

    parser.add_option("--scenarios", action="store_true")
    parser.add_option("--hosts", action="store_true")
    parser.add_option("--test", action="store_true")

    (options, args) = parser.parse_args()

    if options.scenarios:
        find_scenarios()
    if options.hosts:
        find_hosts(args)
    if options.test:
        test()


if __name__ == "__main__":
    main()

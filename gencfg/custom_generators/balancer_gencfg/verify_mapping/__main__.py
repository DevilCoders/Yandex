#!/usr/bin/env python
# coding: utf-8

import os
import sys
import json
import requests
import logging
import retry
import cPickle
from argparse import ArgumentParser

from src import mapping
from search.tools.devops.libs import utils as u


class ServiceError(object):
    NON_GENCFG = "non-gencfg"
    OFFLINE = "offline"
    MULTIPLE = "multiple"
    ORPHANED = "orphaned"
    DELETED = "deleted"
    UNREFERENCED = "unreferenced"
    NOT_IN_CACHE = "not-in-cache"

    ALL = [
        NON_GENCFG,
        OFFLINE,
        MULTIPLE,
        ORPHANED,
        DELETED,
        UNREFERENCED,
        NOT_IN_CACHE,
    ]


@retry.retry(tries=3, delay=5)
def gencfg_group_exists(group_name):
    url = "http://api.gencfg.yandex-team.ru/trunk/groups/{group_name}".format(group_name=group_name)
    logging.info("Checking group `%s` via `%s`", group_name, url)
    r = requests.get(url, timeout=120)
    if r.status_code != 200:
        logging.debug("GenCFG API returned %s", r.status_code)
    return r.status_code != 404


class Service(object):
    """
    Represents GenCfg group (src/mapping.py) bounded to Nanny service
    """
    def __init__(
        self,
        session,
        json=None,
        service_id=None,
        group_name=None,
        tag=None,
    ):
        """
        @param json: object from mapping.py (has prio over other fields)
        @param service_id: Nanny service id
        @param group_name: GenCfg group name
        @param tag: can be either `tags/stable-115-r10` or `stable-115-r20`
        """
        self.session = session
        self.error = None
        self.active_topology = None
        self.balancer_tag = None
        self.balancer_tag_fixed = None
        self.service_id = None
        self.tags = list()
        if json is not None:
            assert json["type"] == "group"

            self.group_name = json["name"]
            self.resolve()
            tag = json["tag"]
            if tag != "trunk":
                self.balancer_tag = Service.parse_tag(tag)
            return

        self.service_id = service_id
        self.group_name = group_name
        self.tags = [Service.parse_tag(tag)]

    @staticmethod
    def parse_tag(tag):
        """
        @param tag: can be either `tags/stable-115-r10` or `stable-115-r20`
        or even parsed tuple (leave as is)
        """
        if isinstance(tag, tuple):
            return tag

        parsed_tag = tag.split("-")
        if len(parsed_tag) != 3:
            logging.error("FATAL: Invalid tag %s, fix it before we can proceed", tag)
            sys.exit(1)

        return int(parsed_tag[1]), int(parsed_tag[2].replace("r", ""))

    @staticmethod
    def print_tag(tag):
        if isinstance(tag, tuple):
            return "stable-{}-r{}".format(tag[0], tag[1])

        if tag.startswith("tags/"):
            return tag.replace("tags/", "")
        return tag

    def resolve(self):
        # {"serviceRefs":[{"id":"production_noapache_man_web_rkub"}]}
        r = self.session.post(
            "http://nanny.yandex-team.ru/api/repo/GetServicesForGencfgGroup/",
            json={
                "groupName": self.group_name,
            },
            timeout=120,
        )
        response = r.json()
        # logging.info("  Response for %s: %s", group_name, json.dumps(response, indent=4))
        service_refs = response["serviceRefs"]
        if len(service_refs) > 1:
            logging.error("Your group %s was referenced in multiple services: %s", self.group_name, service_refs)
            self.error = ServiceError.MULTIPLE
            self.service_id = None
            return None

        if not service_refs:
            self.error = ServiceError.ORPHANED
            self.service_id = None

            if not gencfg_group_exists(self.group_name):
                # even more severe error
                self.error = ServiceError.DELETED

            return None

        self.service_id = service_refs[0]["id"]

    def resolve_active_topology(self):
        """
        Returns json with active topologies
        """
        assert self.service_id

        r = self.session.get(
            "http://nanny.yandex-team.ru/v2/services/{service_id}/active/runtime_attrs".format(
                service_id=self.service_id,
            ),
            timeout=120,
        )
        active_runtime_data = r.json()
        if "content" not in active_runtime_data:
            self.error = ServiceError.OFFLINE
            # print active_runtime_data
            # logging.warning("Dead service: %s", self.service_id)
            return None

        instances = active_runtime_data["content"]["instances"]
        # logging.info("Instances:\n%s", json.dumps(active_runtime_data["content"]["instances"], indent=4))

        if instances["chosen_type"] != "EXTENDED_GENCFG_GROUPS":
            self.error = ServiceError.NON_GENCFG
            return None

        self.active_topology = active_runtime_data["content"]["instances"]["extended_gencfg_groups"]["groups"]
        return self.active_topology

    def recluster(self):
        """
        Walks through list of topologies obtained from active service configuration
        and tries to found given tag `balancer_tag`. Properly reclustered service
        should have full match

        @param service_id: Nanny service id
        @param balancer_tag: tag like 'stable-100-r15'
        @param active_topology: `groups` list from nanny service
        @return tag name to recluster to, or None if no recluster needed
        @rtype str
        """
        parsed_service_tags = []
        for tag in self.active_topology:
            service_tag = tag["release"]
            group_name = tag["name"]
            if self.group_name != group_name:
                # services can contain multiple gencfg groups
                continue

            if service_tag == "trunk":
                logging.error(
                    "Service `%s` maintains active trunk for group `%s`, it cannot be under l7heavy. "
                    "Please fix it before we can proceed",
                    self.service_id,
                    self.group_name,
                )
                return None

            parsed_service_tag = Service.parse_tag(service_tag)
            parsed_service_tags.append(parsed_service_tag)

        parsed_service_tags = sorted(parsed_service_tags, reverse=True)
        assert parsed_service_tags
        top_tag = parsed_service_tags[0]

        if self.balancer_tag == top_tag:
            # yeah, we're already on topmost topology
            if len(parsed_service_tags) > 1:
                # check whether we can drop obsolete tags
                for obsolete_tag in parsed_service_tags[1:]:
                    logging.warning(
                        "You can drop obsolete tag `%s` for group `%s` of service `%s` "
                        "if you already deployed current L7 conf",
                        obsolete_tag,
                        self.group_name,
                        self.service_id,
                    )
            return None

        self.balancer_tag_fixed = Service.print_tag(top_tag)
        return self.balancer_tag_fixed

    def mapping(self):
        return {
            "type": "group",
            "group": self.group_name,
            "tag": self.balancer_tag_fixed if self.balancer_tag_fixed else Service.print_tag(self.balancer_tag),
        }


class Services(object):
    def __init__(self):
        self.group_mapping = mapping.versions
        # mapping: group_name -> Service
        self.services = dict()

        groups_cache_name = "groups_cache.pickle"
        if not os.path.exists(groups_cache_name):
            logging.error(
                "Please put `%s` groups cache file here from recent BUILD_CONFIGS_L7_FILL_CACHE job: "
                "https://testenv.yandex-team.ru/?screen=job_history&database=balancer_config-trunk"
                "&job_name=BUILD_CONFIGS_L7_FILL_CACHE",
                groups_cache_name,
            )
            sys.exit(1)
        with open(groups_cache_name) as f:
            self.groups_cache = cPickle.load(f)

        self.init_session()

    def collect_info(self):
        """
        Main processing routine: collect service info
        """
        for group in self.group_mapping:
            service = Service(self.session, json=group)
            if service.group_name in self.services:
                logging.error(
                    "FATAL: Duplicated groups in mapping: %s, please remove them before you start",
                    service.group_name,
                )
                sys.exit(1)

            self.services[service.group_name] = service
            if not service.service_id:
                continue

            logging.info("Resolved service id: %s", service.service_id)
            if not service.resolve_active_topology():
                logging.info("Service `%s` skipped, no active topology", service.service_id)
                continue

            if service.group_name not in self.groups_cache:
                logging.warning(
                    'Group `%s` for service `%s` is not referenced in groups cache, '
                    'can be dropped from mapping',
                    service.group_name,
                    service.service_id,
                )
                service.error = ServiceError.NOT_IN_CACHE
                continue

            balancer_tag_fixed = service.recluster()
            if balancer_tag_fixed:

                logging.warning(
                    "Service `%s` on `%s` needs to be reclustered to `%s`",
                    service.service_id,
                    service.group_name,
                    balancer_tag_fixed,
                )

    def filter(self, error):
        return [
            service for service in self.services.itervalues()
            if service.error == error
        ]

    def print_list(self, service_list):
        for service in self.sort_services(service_list):
            print "    {:50} | {:50}".format(service.group_name, service.service_id)

    def dump_info(self):
        for status in ServiceError.ALL:
            print "==================="
            print "{}:".format(status)
            self.print_list(self.filter(status))

    def sort_services(self, services_list):
        location_prefixes = [
            "ALL_",
            "MSK_IVA_",
            "MSK_URGB_",
            "MSK_",
            "SAS_",
            "MAN_",
            "VLA_",
        ]

        def swap_prefix(x):
            """
            Turn "MAN_SUGGEST" into "SUGGEST_MAN" for sorting
            """
            for prefix in location_prefixes:
                if x.startswith(prefix):
                    swapped = x[len(prefix):] + "|" + prefix
                    return swapped
            return x

        def compare_services(x, y):
            """
            Comparison function
            """
            sx = swap_prefix(x.group_name)
            sy = swap_prefix(y.group_name)
            if sx < sy:
                return -1
            if sx == sy:
                return 0
            return 1

        return sorted(services_list, cmp=compare_services)

    def dump_mapping(self):
        sorted_services = self.sort_services([service for service in self.services.itervalues()])

        with open("new_mapping.py", "w") as fo:
            fo.write("versions = [\n")
            for service in sorted_services:
                if service.error == ServiceError.DELETED:
                    continue
                if service.error == ServiceError.ORPHANED:
                    continue
                if service.group_name not in self.groups_cache:
                    continue

                line = json.dumps(service.mapping())
                fo.write("    {},\n".format(line))
            fo.write("]\n")

    @staticmethod
    def get_headers(token):
        headers = {
            "Authorization": "OAuth {}".format(token),
            "Accept": "application/json",
            "Accept-Encoding": "identity",
            "Content-Encoding": "application/json",
        }
        return headers

    def init_session(self):
        token = os.getenv("OAUTH_NANNY")
        session = requests.Session()
        session.headers.update(Services.get_headers(token))
        self.session = session

    # FIXME(mvel): we can exclude groups for reclustering that don't change
    # instances list
    def get_instances(self, group_name):
        if not group_name:
            raise Exception("Should pass group name. ")

        request = requests.get(
            "http://api.gencfg.yandex-team.ru/{tag}/searcherlookup/groups/{name}/instances".format(
                tag="tags/stable-113-r145",
                name=group_name,
            ),
            timeout=60,
        )

        return request.text


def parse_cmd():
    parser = ArgumentParser(description="Verify groups mapping")
    u.add_verbose_option(parser)
    return parser.parse_args()


def main():
    args = parse_cmd()
    u.logger_setup(verbose_level=args.verbose)
    services = Services()
    services.collect_info()
    services.dump_info()
    services.dump_mapping()


if __name__ == "__main__":
    main()

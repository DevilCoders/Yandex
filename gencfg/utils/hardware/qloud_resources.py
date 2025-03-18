#!/usr/bin/env python
# coding=utf-8

import json
import os
import pickle
import sys

from collections import defaultdict

import requests
import urllib3

urllib3.disable_warnings()

import qloud
import resources_counter

QLOUD_DOMAINS = ["qloud.yandex-team.ru", "qloud-ext.yandex-team.ru"]


def get_full_qloud_domain(domain):
    if "." in domain:
        assert domain in QLOUD_DOMAINS
        return domain

    for el in QLOUD_DOMAINS:
        if el.split(".")[0] == domain:
            return el

    assert False


class QloudQuotaCounter:
    def __init__(self, abc_api_holder, qloud_domain):
        self._abc_api_holder = abc_api_holder

        self._headers = {'Authorization': 'OAuth {}'.format(qloud.get_qloud_tokent())}
        self._domain = get_full_qloud_domain(qloud_domain)
        self._datacenters = ['IVA', 'MAN', 'MYT', 'SAS', 'VLA']
        self._resources = ['CPU', 'MEMORY_GB', 'IO', 'DISK_GB']

        self._projects_to_abc = None
        self._projects_data = {}

    def cloud(self):
        return self._domain.split(".")[0]

    def load(self, reload=False):
        res_pickle_path = os.path.join(resources_counter.create_resources_dumps_dir(),
                                       "all_qloud_data_%s.pickle" % self.cloud())
        if not reload and os.path.exists(res_pickle_path):
            with file(res_pickle_path) as f:
                data = pickle.load(f)

            self._projects_to_abc, self._projects_data = data
            return

        projects_to_abc = self.get_projects_to_abc()
        projects = self.get_projects()

        for i, project in enumerate(projects):
            sys.stderr.write("QuotaCounter.load %s %s/%s\n" % (self.cloud(), i + 1, len(projects)))
            self.get_project_data(project)

        self._projects_data = json.loads(json.dumps(self._projects_data))  # to get rid of lambda at defaultdict

        with file(res_pickle_path, "wb") as f:
            pickle.dump((self._projects_to_abc, self._projects_data), f)

    @staticmethod
    def _get_usage(project_data):
        res = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))

        for env, usages in project_data.get('usedQuota', {}).items():
            for usage in usages:
                res[usage['hardwareSegment']][usage['resource']][usage['location']] += usage['quota']

        return res

    @staticmethod
    def _get_quota(project_data):
        res = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))

        for quota in project_data['quota']:
            res[quota['hardwareSegment']][quota['resource']][quota['location']] += quota['quota']

        return res

    def get_project_data(self, project):
        if self._projects_data and project in self._projects_data:
            return self._projects_data[project]

        url = 'https://{}/api/v1/project/{}/quota'.format(self._domain, project)
        resp = requests.get(url, headers=self._headers, verify=False)
        resp.raise_for_status()

        project_data = resp.json()

        segments_resource_usage = self._get_usage(project_data)
        segments_resource_quota = self._get_quota(project_data)

        res = {
            "usage": segments_resource_usage,
            "quota": segments_resource_quota
        }

        self._projects_data[project] = res

        return res

    def get_projects(self):
        return sorted(self.get_projects_to_abc().keys())

    def get_projects_to_abc(self):
        if self._projects_to_abc:
            return self._projects_to_abc

        url = "https://{}/api/v1/project/".format(self._domain)

        resp = requests.get(url, headers=self._headers, verify=False)
        resp.raise_for_status()

        """
  {
    "projectName": "abt",
    "description": "QLOUDRES-579",
    "admin": true,
    "environmentStatuses": {
      "DEPLOYED": 4
    },
    "applicationCount": 1,
    "metaInfo": {
      "abcId": 3712,
      "description": "Предоставление инфраструктуры для проведения экспериментов, статистика.\nОбсчет результатов на базе инструментов ABT: наблюдения ABT, LSD-вьюер, sessions viewer, dataset viewer.\n",
      "admins": [],
      "links": []
    }
  },
        """

        projects_to_abc = {}

        for project_data in resp.json():
            project_name = project_data["projectName"]
            if 'abcId' in project_data['metaInfo']:
                project_id = project_data['metaInfo']['abcId']
                project_abc = self._abc_api_holder.get_service_slug(project_id)
                if project_abc is None:
                    project_abc = "abc.%s" % project_id
            else:
                project_abc = "abc_unknown.{}.{}".format(self.cloud(), project_name)

            projects_to_abc[project_name] = project_abc

        self._projects_to_abc = projects_to_abc

        return projects_to_abc

    def get_qloud_segments_size(self):
        hosts_data = requests.get(
            "https://{}/api/v1/hosts/search".format(self._domain),
            headers=self._headers
        ).json()

        hosts_stat = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))

        for host in hosts_data:
            cpu = host["cpu"]
            mem = host["memoryBytes"] / (1024 * 1024 * 1024)
            dc = host.get("dataCenter", "unknown")
            segment = host.get("segment", "unknown")

            hosts_stat[dc][segment]["cpu"] += cpu
            hosts_stat[dc][segment]["mem"] += mem

        res = resources_counter.ResourcesCounter()
        for dc, dc_data in hosts_stat.iteritems():
            for segment, segment_data in dc_data.iteritems():
                temp = resources_counter.empty_resources()

                temp["cloud"] = self.cloud()
                temp["dc"] = dc
                temp["segment"] = segment
                temp.update(segment_data)

                res.add_resource(temp)

        return res

    def count(self):
        projects = self.get_projects()
        projects_to_abc = self.get_projects_to_abc()

        cloud = self.cloud()

        services_quota = defaultdict(resources_counter.ResourcesCounter)
        services_allocations = defaultdict(resources_counter.ResourcesCounter)

        services_quota_native = defaultdict(resources_counter.ResourcesCounter)
        services_allocations_native = defaultdict(resources_counter.ResourcesCounter)

        for i, project in enumerate(projects):
            project_data = self.get_project_data(project)
            if not project_data:
                continue

            quota = resources_counter.ResourcesCounter()
            quota.add_resources_from_qloud(cloud=cloud, data=project_data.get("quota"))

            allocations = resources_counter.ResourcesCounter()
            allocations.add_resources_from_qloud(cloud=cloud, data=project_data.get("usage"))

            abc_project = projects_to_abc.get(project,
                                              "abc_unknown.{}.{}".format(cloud, project))

            services_quota[abc_project] += quota
            services_allocations[abc_project] += allocations

            key = {
                "abc_service": abc_project,
                "project": project,
            }
            key = frozenset(key.iteritems())

            assert key not in services_quota_native
            assert key not in services_allocations_native

            services_quota_native[key] += quota
            services_allocations_native[key] += allocations

        segments_size = self.get_qloud_segments_size()

        return segments_size, services_quota, services_allocations, services_quota_native, services_allocations_native


def get_qloud_resources_data(abc_api_holder, config, reload=False):
    total_segments_size = resources_counter.ResourcesCounter()

    total_services_quota = defaultdict(resources_counter.ResourcesCounter)
    total_services_allocations = defaultdict(resources_counter.ResourcesCounter)

    total_services_quota_native = defaultdict(resources_counter.ResourcesCounter)
    total_services_allocations_native = defaultdict(resources_counter.ResourcesCounter)

    for domain in QLOUD_DOMAINS:
        quota_counter = QloudQuotaCounter(abc_api_holder, domain)
        quota_counter.load(reload=reload)

        segments_size, services_quota, services_allocations, services_quota_native, services_allocations_native = quota_counter.count()
        total_segments_size += segments_size

        total_services_quota = resources_counter.join_resources_map(total_services_quota, services_quota)
        total_services_allocations = resources_counter.join_resources_map(total_services_allocations,
                                                                          services_allocations)

        total_services_quota_native = resources_counter.join_resources_map(total_services_quota_native,
                                                                           services_quota_native)
        total_services_allocations_native = resources_counter.join_resources_map(total_services_allocations_native,
                                                                                 services_allocations_native)

    segments_to_skip = config.get("qloud", {}).get("segments_to_skip", [])

    resources_counter.filter_out_segments(total_segments_size, segments_to_skip)
    resources_counter.filter_out_segments(total_services_quota, segments_to_skip)
    resources_counter.filter_out_segments(total_services_allocations, segments_to_skip)
    resources_counter.filter_out_segments(total_services_quota_native, segments_to_skip)
    resources_counter.filter_out_segments(total_services_allocations_native, segments_to_skip)

    return total_segments_size, total_services_quota, total_services_allocations, total_services_quota_native, total_services_allocations_native

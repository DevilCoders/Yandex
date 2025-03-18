"""Local storage of dispenser quotas for groups"""

import os
import json
import collections

import gencfg

from gaux.aux_staff import unwrap_dpts


class Dispenser(object):
    """
    Storage with mapping
    {
        project_key ->
            location ->
                segment ->
                    quota_spec_key ->
                        resource_key ->
                            service_key ->
                                quota
    }
    """

    def __init__(self, db):
        self.db = db
        self.quotas = {}
        self.modified = False
        self.quotas = {}
        self.locations = {}
        self.segments = {}
        self.projects = {}
        self.resourses = {}
        self.group_by_project = {}

        if self.db.version < '2.2.55':
            return

        self.QUOTAS_FILE = os.path.join(self.db.PATH, 'dispenser_quotas.json')
        self.LOCATIONS_FILE = os.path.join(self.db.PATH, 'dispenser_locations.json')
        self.SEGMENTS_FILE = os.path.join(self.db.PATH, 'dispenser_segments.json')
        self.PROJECTS_FILE = os.path.join(self.db.PATH, 'dispenser_projects.json')
        self.RESOURCES_FILE = os.path.join(self.db.PATH, 'dispenser_resources.json')

        with open(self.QUOTAS_FILE, 'r') as rfile:
            self.quotas = json.load(rfile)

        with open(self.LOCATIONS_FILE, 'r') as rfile:
            self.locations = json.load(rfile)

        with open(self.SEGMENTS_FILE, 'r') as rfile:
            self.segments = json.load(rfile)

        with open(self.PROJECTS_FILE, 'r') as rfile:
            self.projects = json.load(rfile)

        with open(self.RESOURCES_FILE, 'r') as rfile:
            self.resourses = json.load(rfile)

        self.group_by_project = self.load_project_groups()

    def load_project_groups(self):
        projects = collections.defaultdict(list)
        for group in self.db.groups.get_groups():
            if group.card.dispenser.project_key is None:
                continue
            projects[group.card.dispenser.project_key].append(group)
        return projects

    def mark_as_modified(self):
        self.modified = True

    def purge(self):
        self.quotas = {}
        self.locations = {}
        self.segments = {}
        self.projects = {}
        self.resourses = {}

    def to_json(self):
        return {
            'quotas': self.quotas,
            'locations': self.locations,
            'segments': self.segments,
            'projects': self.projects,
            'resources': self.resourses
        }

    def from_json(self, jsoned):
        self.quotas = jsoned['quotas']
        self.locations = jsoned['locations']
        self.segments = jsoned['segments']
        self.projects = jsoned['projects']
        self.resourses = jsoned['resources']

    def update(self, smart=False):
        if self.db.version < '2.2.55':
            return

        if not self.modified:
            return

        with open(self.QUOTAS_FILE, 'w') as wfile:
            json.dump(self.quotas, wfile, indent=4)

        with open(self.LOCATIONS_FILE, 'w') as wfile:
            json.dump(self.locations, wfile, indent=4)

        with open(self.SEGMENTS_FILE, 'w') as wfile:
           json.dump(self.segments, wfile, indent=4)

        with open(self.PROJECTS_FILE, 'w') as wfile:
            json.dump(self.projects, wfile, indent=4)

        with open(self.RESOURCES_FILE, 'w') as wfile:
            json.dump(self.resourses, wfile, indent=4)

    def fast_check(self, timeout):
        pass

    def get_leaf_projects(self, project_key=None):
        if project_key is None:
            return {k: v for k, v in self.projects.items() if not v['subprojectKeys']}

        if not self.projects[project_key]['subprojectKeys']:
            return [project_key]

        leaf_project = []
        for subproject_key in self.projects[project_key]['subprojectKeys']:
            leaf_project += self.get_leaf_projects(subproject_key)
        return leaf_project

    def get_projects(self):
        return self.projects

    def get_project_acl(self, project_key):
        project = self.projects[project_key]

        m_persons = project['members']['persons']
        m_persons += unwrap_dpts(project['members']['yandexGroups'])

        r_persons = project['responsibles']['persons']
        r_persons += unwrap_dpts(project['responsibles']['yandexGroups'])

        return set(m_persons + r_persons)

    def get_quotas(self):
        return self.quotas

    def get_keyed_quotas(self):
        keyed_quotas = {}
        for project_key in self.quotas:
            keyed_quotas[project_key] = self._reduce_tree_filter(self.quotas[project_key], [None] * 5)
        return keyed_quotas

    def get_locations(self):
        return self.locations

    def get_segments(self):
        return self.segments

    def get_resources(self):
        return self.resourses

    def get_groups_by_project(self, project_key=None, nocache=False):
        if nocache:
            self.group_by_project = self.load_project_groups()

        if project_key is None:
            return self.group_by_project
        return self.group_by_project[project_key]

    def get_project_quotas(self, project_key=None, location_key=None, segment_key=None, quota_key=None,
                           resource_key=None, service_key='gencfg'):
        filter_keys = (project_key, location_key, segment_key, quota_key, resource_key, service_key)
        filtered_quotas = self._reduce_tree_filter(self.quotas, filter_keys)

        return filtered_quotas

    def get_sum_project_quota(self, project_key=None, location_key=None, segment_key=None, quota_key=None,
                              resource_key=None, service_key='gencfg'):
        filter_keys = (project_key, location_key, segment_key, quota_key, resource_key, service_key)
        filtered_quotas = self.get_project_quotas(*filter_keys)

        sum_quotas = {}
        for path_keys, quotas in filtered_quotas.items():
            resource_key = path_keys[4]
            if resource_key not in sum_quotas:
                sum_quotas[resource_key] = {'actual': 0, 'actual_unit': None}

            sum_quotas[resource_key]['actual'] += quotas['actual']
            sum_quotas[resource_key]['actual_unit'] = quotas['actual_unit']
        return sum_quotas

    def get_max_project_quota(self, project_key, location_key, segment_key, quota_key=None, resource_key=None,
                              service_key='gencfg'):
        filter_keys = (project_key, location_key, segment_key, quota_key, resource_key, service_key)
        filtered_quotas = self.get_project_quotas(*filter_keys)

        max_quotas = {}
        for path_keys, quotas in filtered_quotas.items():
            resource_key = path_keys[4]
            if resource_key not in max_quotas:
                max_quotas[resource_key] = {'max': 0, 'max_unit': None}

            max_quotas[resource_key]['max'] = quotas['max']
            max_quotas[resource_key]['max_unit'] = quotas['max_unit']
        return max_quotas

    @staticmethod
    def _reduce_tree_filter(obj, filter_keys, level=0, path_prefix=None):
        path_prefix = path_prefix or tuple()

        data = {}
        for key in obj:
            if filter_keys[level] is not None and filter_keys[level] != key:
                continue
            if level + 1 == len(filter_keys):
                full_path = path_prefix + (key,)
                data[full_path] = obj[key]
            else:
                data.update(Dispenser._reduce_tree_filter(obj[key], filter_keys, level+1, path_prefix + (key,)))
        return data


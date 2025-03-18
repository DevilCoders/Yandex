#!/skynet/python/bin/python
# coding: utf8
import time
import json
import logging
import collections

import requests


class Mapping(object):
    D_TO_G_RES = {
        'cpu': 'power',
        'memory': 'memory_guarantee',
        'hdd': 'disk',
        'ssd': 'ssd'
    }
    G_TO_D_RES = {v: k for k, v in D_TO_G_RES.items()}

    D_TO_G_LOC = {
        'MAN': 'MAN',
        'SAS': 'SAS',
        'VLA': 'VLA',
        'IVA': 'IVA',
        'MYT': 'MYT'
    }
    G_TO_D_LOC = {v: k for k, v in D_TO_G_LOC.items()}

    D_TO_G_SEG = {
        'common': 'normal',
        'personal': 'personal',
        'sox': 'sox'
    }
    G_TO_D_SEG = {v: k for k, v in D_TO_G_SEG.items()}

    UNIT = {
        'cpu': 'PERMILLE_CORES',
        'memory': 'BYTE',
        'hdd': 'BYTE',
        'ssd': 'BYTE'
    }

    @staticmethod
    def mapping(MAP_DICT, key):
        return MAP_DICT.get(key, key)

    @staticmethod
    def g_res(key):
        return Mapping.mapping(Mapping.D_TO_G_RES, key)

    @staticmethod
    def d_res(key):
        return Mapping.mapping(Mapping.G_TO_D_RES, key)

    @staticmethod
    def g_loc(key):
        return Mapping.mapping(Mapping.D_TO_G_LOC, key)

    @staticmethod
    def d_loc(key):
        return Mapping.mapping(Mapping.G_TO_D_LOC, key)

    @staticmethod
    def g_seg(key):
        return Mapping.mapping(Mapping.D_TO_G_SEG, key)

    @staticmethod
    def d_seg(key):
        return Mapping.mapping(Mapping.G_TO_D_SEG, key)

    @staticmethod
    def d_unit(key):
        return Mapping.mapping(Mapping.UNIT, key)


def _get_curdb():
    from core.db import CURDB
    return CURDB


class Dispenser(object):
    API_URL_TESTING = 'https://dispenser.test.yandex-team.ru/clouds/api{}'
    API_URL_PRODUCTION = "https://dispenser.yandex-team.ru/api{}"

    HTTP_METHODS = {
        'GET': requests.get,
        'POST': requests.post,
        'PUT': requests.put,
        'DELETE': requests.delete
    }

    class QuotingMode(object):
        DEFAULT = 'DEFAULT'
        ENTITIES_ONLY = 'ENTITIES_ONLY'
        SYNCHRONIZATION = 'SYNCHRONIZATION'

    class Mode(object):
        ROLLBACK_ON_ERROR = 'ROLLBACK_ON_ERROR'
        IGNORE_UNKNOWN_ENTITIES_AND_USAGES = 'IGNORE_UNKNOWN_ENTITIES_AND_USAGES'

    class OperationType(object):
        FORCE_ACQUIRE_RESOURCE = 'FORCE_ACQUIRE_RESOURCE'
        RELEASE_RESOURCE = 'RELEASE_RESOURCE'
        CREATE_ENTITY = 'CREATE_ENTITY'
        RELEASE_ENTITY = 'RELEASE_ENTITY'
        SHARE_ENTITY = 'SHARE_ENTITY'
        RELEASE_ENTITY_SHARING = 'RELEASE_ENTITY_SHARING'
        RELEASE_ALL_ENTITY_SHARINGS = 'RELEASE_ALL_ENTITY_SHARINGS'

    class Units(object):
        DEFAULT = ''

    def __init__(self, token, testing=False):
        self.token = token
        self.api_url = self.API_URL_TESTING if testing else self.API_URL_PRODUCTION
        self.batch = {}

    def request(self, api_path, data=None, json_data=None, headers=None, method='GET', not_retry=False):
        for delay in (1, 3, 10, -1):
            try:
                data = data or {}
                http_method = self.HTTP_METHODS[method.upper()]

                for key, value in data.items():
                    sep = '?' if '?' not in api_path else '&'
                    api_path = '{}{}{}={}'.format(api_path, sep, key, value)

                full_headers = {'Authorization': 'OAuth {}'.format(self.token)}
                full_headers.update(headers or {})

                http_method_kwargs = {
                    'url': self.api_url.format(api_path),
                    'headers': full_headers,
                    'verify': False
                }
                if json_data and method.upper() in ('POST', 'PUT'):
                    http_method_kwargs['headers'].update({
                        'Content-type': 'application/json',
                        'Accept': 'application/json'
                    })
                    http_method_kwargs.update({'data': json.dumps(json_data)})

                logging.debug('Dispenser {}: {}'.format(http_method.__name__, http_method_kwargs))
                response = http_method(**http_method_kwargs)

                if response.status_code != 200:
                    #print(http_method.__name__, http_method_kwargs)
                    raise ValueError('Status code {} != 200 ({})'.format(response.status_code, response.content))
                return response.json()
            except Exception as e:
                if delay < 0 or not_retry:
                    raise
                logging.warning('{} {} -> {}: {}, retry (delay: {})'.format(method, api_path, type(e), e, delay))
                time.sleep(delay)

    def request_get(self, api_path, data=None, headers=None, not_retry=False):
        return self.request(api_path, data=data, headers=headers, method='GET', not_retry=not_retry)

    def request_post(self, api_path, data=None, json_data=None, headers=None, not_retry=False):
        return self.request(api_path, data=data, json_data=json_data, headers=headers, method='POST',
                            not_retry=not_retry)

    def request_put(self, api_path, data=None, json_data=None, headers=None, not_retry=False):
        return self.request(api_path, data=data, json_data=json_data, headers=headers, method='PUT',
                            not_retry=not_retry)

    def request_delete(self, api_path, data=None, headers=None, not_retry=False):
        return self.request(api_path, data=data, headers=headers, method='DELETE', not_retry=not_retry)

    def projects(self, project_key=None, query=None):
        query = query or {'showPersons': 'true'}
        if project_key is not None:
            return self.request_get('/v1/projects/{}'.format(project_key), data=query)
        return self.request_get('/v1/projects', data=query)

    def make_project_data_json(self, project_key, abc_service_id, parent_project_key, responsibles,
                               project_name=None, project_desc=None, members=None, subproject_keys=None):
        return {
            "key": project_key,
            "name": project_name or project_key,
            "description": project_desc or "",
            "abcServiceId": abc_service_id,
            "responsibles": {
                "persons": responsibles,
                "yandexGroups": {
                    "departments": [],
                    "services": [],
                    "serviceroles": [],
                    "wiki": []
                }
            },
            "members": {
                "persons": members,
                "yandexGroups": {
                    "departments": [],
                    "services": [],
                    "serviceroles": [],
                    "wiki": []
                }
            },
            "parentProjectKey": parent_project_key,
            "subprojectKeys": subproject_keys or []
        }

    def projects_create_subproject(self, parent_project_key, project_data_json):
        return self.request_put('/v1/projects/{}/create-subproject'.format(parent_project_key),
                                json_data=project_data_json)

    def projects_update_subproject(self, project_key, project_data_json):
        return self.request_post('/v1/projects/{}'.format(project_key), json_data=project_data_json)

    def projects_attach_members(self, project_key, json_list_with_logins):
        return self.request_post('/v1/projects/{}/attach-members'.format(project_key), json_data=json_list_with_logins)

    def projects_detach_members(self, project_key, json_list_with_logins):
        return self.request_post('/v1/projects/{}/detach-members'.format(project_key), json_data=json_list_with_logins)

    def projects_attach_responsibles(self, project_key, json_list_with_logins):
        return self.request_post('/v1/projects/{}/attach-responsibles'.format(project_key),
                                 json_data=json_list_with_logins)

    def projects_detach_responsibles(self, project_key, json_list_with_logins):
        return self.request_post('/v1/projects/{}/detach-responsibles'.format(project_key),
                                 json_data=json_list_with_logins)

    def services(self, service_key=None):
        if service_key is not None:
            return self.request_get('/v1/services/{}'.format(service_key))
        return self.request_get('/v1/services/all')

    def services_create(self, service_key, service_data_json):
        return self.request_put('/v1/services/{}'.format(service_key), json_data=service_data_json)

    def services_update(self, service_key, service_data_json):
        return self.request_post('/v1/services/{}'.format(service_key), json_data=service_data_json)

    def services_delete(self, service_key):
        return self.request_delete('/v1/services/{}'.format(service_key))

    def services_attach_admins(self, service_key, json_list_with_logins):
        return self.request_post('/v1/projects/{}/attach-admins'.format(service_key), json_data=json_list_with_logins)

    def services_detach_admins(self, service_key, json_list_with_logins):
        return self.request_post('/v1/projects/{}/detach-admins'.format(service_key), json_data=json_list_with_logins)

    def resources(self, service_key='gencfg', resource_key=None):
        if resource_key is None:
            return self.request_get('/v1/services/{}/resources'.format(service_key))
        return self.request_get('/v1/services/{}/resources/{}'.format(service_key, resource_key))

    def resources_create(self, service_key, resource_key, resource_data_json):
        return self.request_put('/v1/services/{}/resources/{}'.format(service_key, resource_key),
                                json_data=resource_data_json)

    def resources_update(self, service_key, resource_key, resource_data_json):
        return self.request_post('/v1/services/{}/resources/{}'.format(service_key, resource_key),
                                 json_data=resource_data_json)

    def resources_delete(self, service_key, resource_key):
        return self.request_delete('/v1/services/{}/resources/{}'.format(service_key, resource_key))

    def quota_specifications(self, service_key, resource_key, quota_spec_key):
        return self.request_get('/v1/quota-specifications/{}/{}/{}'.format(service_key, resource_key, quota_spec_key))

    def quota_specifications_create(self, service_key, resource_key, quota_spec_key, quota_spec_data_json):
        return self.request_put('/v1/quota-specifications/{}/{}/{}'.format(service_key, resource_key, quota_spec_key),
                                json_data=quota_spec_data_json)

    def quota_specifications_update(self, service_key, resource_key, quota_spec_key, quota_spec_data_json):
        return self.request_post('/v1/quota-specifications/{}/{}/{}'.format(service_key, resource_key, quota_spec_key),
                                 json_data=quota_spec_data_json)

    def quota_specifications_delete(self, service_key, resource_key, quota_spec_key):
        return self.request_delete('/v1/quota-specifications/{}/{}/{}'.format(service_key, resource_key,
                                   quota_spec_key))

    def quotas(self, query=None):
        return self.request_get('/v2/quotas', data=query)

    def quotas_update(self, service_key, resource_key, quota_spec_key, quota_data_json):
        return self.request_post('/v1/quotas/{}/{}/{}'.format(service_key, resource_key, quota_spec_key),
                                 json_data=quota_data_json)

    def change_quotas_new_batch(self, mode, service_key):
        self.batch = {
            'mode': mode,
            'serviceKey': service_key,
            'operations': []
        }

    def make_action(self, op_type, **kwargs):
        if op_type in (self.self.OperationType.FORCE_ACQUIRE_RESOURCE, self.self.OperationType.RELEASE_RESOURCE):
            return {
                'resourceKey': kwargs['resource_key'],
                'amount': {
                    'value': kwargs['value'],
                    'unit': kwargs['unit']
                },
                'segments': kwargs['segments']
            }
        elif op_type in (self.self.OperationType.CREATE_ENTITY,):
            action = {
                'key': kwargs['key'],
                'specificationKey': kwargs['spec_key'],
                'dimensions': []
            }
            for dimension in kwargs['dimensions']:
                action['dimensions'].append({
                    'resourceKey': dimension[0],
                    'amount': {
                        'value': dimension[1],
                        'unit': dimension[2]
                    }
                })
        elif op_type in (self.self.OperationType.RELEASE_ENTITY, self.self.OperationType.RELEASE_ALL_ENTITY_SHARINGS):
            return {
                'key': kwargs['key'],
                'specificationKey': kwargs['spec_key']
            }
        elif op_type in (self.self.OperationType.SHARE_ENTITY, self.self.OperationType.RELEASE_ENTITY_SHARING):
            return {
                'entity': kwargs['entity_ref'],
                'usagesCount': kwargs['usage_count']
            }
        else:
            return {}

    def make_operation(self, op_id, op_type, action, login, project_key):
        return {
            'id': op_id,
            'operation': {
                'type': op_type,
                'action': action,
                'performer': {
                    'login': login,
                    'projectKey': project_key
                }
            }
        }

    def change_quotas_acquire_resource(self, project_key, resource_key, value, unit, segments):
        action = self.make_action(self.OperationType.FORCE_ACQUIRE_RESOURCE, resource_key=resource_key,
                                  value=value, unit=unit, segments=segments)
        operation = self.make_operation(len(self.batch['operations']) + 1, self.OperationType.FORCE_ACQUIRE_RESOURCE,
                                        action, '', project_key)
        self.batch['operations'].append(operation)

    def change_quotas_release_resource(self, project_key, resource_key, value, unit, segments):
        action = self.make_action(self.OperationType.RELEASE_RESOURCE, resource_key=resource_key,
                                  value=value, unit=unit, segments=segments)
        operation = self.make_operation(len(self.batch['operations']) + 1, self.OperationType.RELEASE_RESOURCE,
                                        action, '', project_key)
        self.batch['operations'].append(operation)

    def change_quotas_create_enity(self, project_key, key, spec_key, dimensions):
        action = self.make_action(self.OperationType.CREATE_ENTITY, key=key, spec_key=spec_key, dimensions=dimensions)
        operation = self.make_operation(len(self.batch['operations']) + 1, self.OperationType.CREATE_ENTITY,
                                        action, '', project_key)
        self.batch['operations'].append(operation)

    def change_quotas_release_entity(self, project_key, key, spec_key):
        action = self.make_action(self.OperationType.RELEASE_ENTITY, key=key, spec_key=spec_key)
        operation = self.make_operation(len(self.batch['operations']) + 1, self.OperationType.RELEASE_ENTITY,
                                        action, '', project_key)
        self.batch['operations'].append(operation)

    def change_quotas_release_all_entity_sharings(self, project_key, key, spec_key):
        action = self.make_action(self.OperationType.RELEASE_ALL_ENTITY_SHARINGS, key=key, spec_key=spec_key)
        operation = self.make_operation(len(self.batch['operations']) + 1, self.OperationType.RELEASE_ALL_ENTITY_SHARINGS,
                                        action, '', project_key)
        self.batch['operations'].append(operation)

    def change_quotas_share_entity(self, project_key, entity_ref, usage_count):
        action = self.make_action(self.OperationType.SHARE_ENTITY, entity_ref=entity_ref, usage_count=usage_count)
        operation = self.make_operation(len(self.batch['operations']) + 1, self.OperationType.SHARE_ENTITY,
                                        action, '', project_key)
        self.batch['operations'].append(operation)

    def change_quotas_release_entity_sharing(self, project_key, entity_ref, usage_count):
        action = self.make_action(self.OperationType.RELEASE_ENTITY_SHARING, entity_ref=entity_ref, usage_count=usage_count)
        operation = self.make_operation(len(self.batch['operations']) + 1, self.OperationType.RELEASE_ENTITY_SHARING,
                                        action, '', project_key)
        self.batch['operations'].append(operation)

    def change_quotas_execute_batch(self):
        if not self.batch:
            return None

        response = self.request_post('/v1/change-quotas', json_data=self.bacth)
        self.batch = {}
        return response

    def make_sync_state_quotas_obj(self, project_key, resource_key, quota_spec_key, segments, actual_value=None,
                                   actual_unit=None, max_value=None, max_unit=None):
        data = {
            'projectKey': project_key,
            'resourceKey': resource_key,
            'quotaSpecKey': quota_spec_key,
            'segments': segments
        }
        if actual_value is not None and actual_unit is not None:
            data['actual'] = {
                'value': actual_value,
                'unit': actual_unit
            }
        elif max_value is not None and max_unit is not None:
            data['max'] = {
                'value': max_value,
                'unit': max_unit
            }

        if 'actual' not in data and 'max' not in data:
            raise ValueError('Need (actual_value, actual_unit) or (max_value, max_unit) params')

        return data

    def sync_state_quotas(self, service_key, list_sync_state_quotas_objs):
        return self.request_post('/v1/services/{}/sync-state/quotas/set'.format(service_key),
                                 json_data=list_sync_state_quotas_objs)

    def entity_specifications(self, service_key=None, entity_spec_key=None, query=None):
        if service_key is not None and entity_spec_key is not None:
            return self.request_get('/v1/entity-specifications/{}/{}'.format(service_key, entity_spec_key))
        return self.request_get('/v1/entity-specifications', data=query)

    def entities(self, service_key=None, entity_spec_key=None, entity_key=None, query=None):
        if service_key is not None and entity_spec_key is not None and entity_key is not None:
            return self.request_get('/v1/entities/{}/{}/{}'.format(service_key, entity_spec_key, entity_key))
        return self.request_get('/v1/entities', data=query)

    def segmentations(self, segmentation_key=None):
        if segmentation_key is not None:
            return self.request_get('/v1/segmentations/{}'.format(segmentation_key))
        return self.request_get('/v1/segmentations')

    def segmentations_create(self, segmentations_json_data):
        return self.request_post('/v1/segmentations', json_data=segmentations_json_data)

    def segmentations_update(self, segmentation_key, segmentations_json_data):
        return self.request_put('/v1/segmentations/{}'.format(segmentation_key), json_data=segmentations_json_data)

    def segmentations_delete(self, segmentation_key):
        return self.request_delete('/v1/segmentations/{}'.format(segmentation_key))

    def segments(self, segmentation_key, segment_key=None):
        if segment_key is not None:
            return self.request_get('/v1/segmentations/{}/segments/{}'.format(segmentation_key, segment_key))
        return self.request_get('/v1/segmentations/{}/segments'.format(segmentation_key))

    def segments_create(self, segmentation_key, segment_json_data):
        return self.request_post('/v1/segmentations/{}/segments'.format(segmentation_key), json_data=segment_json_data)

    def segments_update(self, segmentation_key, segment_key, segment_json_data):
        return self.request_put('/v1/segmentations/{}/segments/{}'.format(segmentation_key, segment_key),
                                json_data=segment_json_data)

    def segments_delete(self, segmentation_key, segment_key):
        return self.request_delete('/v1/segmentations/{}/segments/{}'.format(segmentation_key, segment_key))

    def resources_segmentations(self, service_key, resource_key):
        return self.request_get('/v1/services/{}/resources/{}/segmentations'.format(service_key, resource_key))

    def resources_segmentationso_set(self, service_key, resource_key, list_segmentations_json_data):
        return self.request_put('/v1/services/{}/resources/{}/segmentations'.format(service_key, resource_key),
                                json_data=list_segmentations_json_data)

    def resource_groups(self, service_key, resource_group_key=None):
        if resource_group_key is not None:
            return self.request_get('/v1/services/{}/resource-groups/{}'.format(service_key, resource_group_key))
        return self.request_get('/v1/services/{}/resource-groups'.format(service_key))

    def resource_groups_create(self, service_key, resource_groups_json_data):
        return self.request_post('/v1/services/{}/resource-groups'.format(service_key),
                                 json_data=resource_groups_json_data)

    def resource_groups_update(self, service_key, resource_group_key, resource_groups_json_data):
        return self.request_put('/v1/services/{}/resource-groups/{}'.format(service_key, resource_group_key),
                                json_data=resource_groups_json_data)

    def resource_groups_delete(self, service_key, resource_group_key):
        return self.request_delete('/v1/services/{}/resource-groups/{}'.format(service_key, resource_group_key))


def load_dispenser_data(dispenser=None):
    """
    Return dict
    {
        "quotas": {
            "project_key": {
                "location_key": {
                    "segment_key": {
                        "quota_specification_key": {
                            "resource_key": {
                                "service_key": {
                                    "actual": <int value>,
                                    "actual_unti": <str value>,
                                    "max": <int value>
                                    "max_unit": <str value>
                                }
                            }
                        }
                    }
                }
            }
        },
        "locations": {
            "location_key": <Dispenser segment object>
        },
        "segments": {
            "segment_key": <Dispenser segment object>
        },
        "projects": {
            "project_key": <Dispenser project object>
        },
        "resources": {
            "resource_key": <Dispenser resource object>
        }
    }
    """
    def prepare_project_path(dct, location, segment, spec, resource, service):
        current_level_dct = dct
        for key in (location, segment, spec, resource, service):
            if key not in current_level_dct:
                current_level_dct[key] = {}
            current_level_dct = current_level_dct[key]

        if not dct[location][segment][spec][resource][service]:
            dct[location][segment][spec][resource][service] = []

    dispenser = dispenser or Dispenser('<readonly>')

    locations = {x['key']: x for x in dispenser.segments('locations')['result']}
    segments = {x['key']: x for x in dispenser.segments('gencfg_segments')['result']}
    projects = {x['key']: x for x in dispenser.projects()['result']}
    resources = {x['key']: x for x in dispenser.resources()['result']}

    quotas = collections.defaultdict(dict)
    for record in dispenser.quotas()['result']:
        if len(record['key']['segmentKeys']) != 2:
            continue
        elif record['key']['serviceKey'] != 'gencfg':
            continue

        project_key = record['key']['projectKey']

        spec = record['key']['quotaSpecKey']
        resource = record['key']['resourceKey']
        service = record['key']['serviceKey']

        max_value = record['max']
        max_unit = Mapping.d_unit(resource)
        actual_value = record['actual']
        actual_unit = Mapping.d_unit(resource)

        location, segment = None, None
        for seg in record['key']['segmentKeys']:
            if seg in locations:
                location = seg
            if seg in segments:
                segment = seg

        if location is None or segment is None:
            logging.info('Not found segments in quota object')
            continue

        prepare_project_path(quotas[project_key], location, segment, spec, resource, service)
        quotas[project_key][location][segment][spec][resource][service] = {
            'actual': actual_value,
            'actual_unit': actual_unit,
            'max': max_value,
            'max_unit': max_unit
        }

    data = {
        'quotas': quotas,
        'locations': locations,
        'segments': segments,
        'projects': projects,
        'resources': resources
    }
    return data


def load_projects_from_db(db=None):
    projects = collections.defaultdict(list)
    for group in db.groups.get_groups():
        if group.card.dispenser.project_key is None:
            continue
        projects[group.card.dispenser.project_key].append(group)
    return projects


def sum_groups_allocated(groups, db=None):
    """
    Return dict
    {
        (loc_key, seg_key, spec_key, res_key): <sum_resource>
    }
    """
    def group_guarantee(group, location_key, segment_key, resource_key):
        location_key = Mapping.g_loc(location_key)  # TODO: Remove MAPPING
        segment_key = Mapping.g_seg(segment_key)  # TODO: Remove MAPPING
        resource_key = Mapping.g_res(resource_key)

        if group.card.properties.security_segment != segment_key:
            return 0
        elif resource_key not in ('power', 'memory_guarantee', 'disk', 'ssd'):
            return 0

        sum_guarantee = 0
        for instance in group.get_instances():
            if instance.host.dc.upper() == location_key:
                if resource_key == 'power':
                    sum_guarantee += int(instance.power / 40. * 1000)
                else:
                    sum_guarantee += getattr(group.card.reqs.instances, resource_key).value
        return sum_guarantee

    result = {}
    for group in groups:
        for loc_key in db.dispenser.get_locations():
            for seg_key in db.dispenser.get_segments():
                for res_key in db.dispenser.get_resources():
                    spec_key = '{}-quota'.format(res_key)
                    if (loc_key, seg_key, spec_key, res_key) not in result:
                        result[(loc_key, seg_key, spec_key, res_key)] = 0
                    result[(loc_key, seg_key, spec_key, res_key)] += group_guarantee(group, loc_key, seg_key, res_key)
    return result


def calc_projects_allocated(db=None, project_key=None):
    db = db or _get_curdb()
    dispenser_projects = db.dispenser.get_groups_by_project()

    projects_allocated = {}
    if project_key is not None:
        project_groups = []
        for proj_key in db.dispenser.get_leaf_projects(project_key):
            project_groups += dispenser_projects[proj_key]
        projects_allocated[project_key] = sum_groups_allocated(project_groups, db)
    else:
        for proj_key, project_groups in dispenser_projects.items():
            projects_allocated[proj_key] = sum_groups_allocated(project_groups, db)
    return projects_allocated


def check_projects_allocated(db=None, project_key=None, fast_raise=False):
    db = db or _get_curdb()
    projects_allocated = calc_projects_allocated(db, project_key)

    error_projects = []
    for proj_key in projects_allocated:
        for path_keys, value in projects_allocated[proj_key].items():
            loc_key, seg_key, spec_key, res_key = path_keys
            resources_max = db.dispenser.get_max_project_quota(proj_key, loc_key, seg_key)

            if resources_max[res_key]['max'] < value:
                message = '[{}:{}:{}] Sum allocated {} greate than max dispenser quota ({} > {})'.format(
                    proj_key, loc_key, seg_key, res_key, value, resources_max[res_key]['max']
                )
                error_projects.append((
                    message, proj_key, loc_key, seg_key, res_key, value, resources_max[res_key]['max']
                ))
                if fast_raise:
                    raise ValueError(message)
    return error_projects


def check_projects_acl(db=None, project_key=None, fast_raise=False):
    from gaux.aux_staff import unwrap_dpts

    if project_key is not None:
        projects_groups = {project_key: db.dispenser.get_groups_by_project(project_key)}
    else:
        projects_groups = db.dispenser.get_groups_by_project()

    error_projects = []
    for project_key, project_groups in projects_groups.items():
        project_acl = db.dispenser.get_project_acl(project_key)
        for group in project_groups:
            group_acl = set(unwrap_dpts(group.card.owners))
            if not len(project_acl & group_acl):
                message = 'No common owners for {}({}) and {}({})'.format(
                    project_key, ','.join(project_acl),
                    group.card.name, ','.join(group_acl)
                )
                error_projects.append((message, project_key, project_acl, group.card.name, group_acl))
                if fast_raise:
                    raise ValueError(message)
    return error_projects


def check_groups_project(db=None, fast_raise=False):
    error_groups = []

    leaf_projects = db.dispenser.get_leaf_projects().keys()
    for group in db.groups.get_groups():
        if group.card.dispenser.project_key and group.card.dispenser.project_key not in leaf_projects:
            message = 'Group {} has no leaf dispenser project_key {}'.format(
                group.card.name, group.card.dispenser.project_key
            )
            error_groups.append((message, group.card.name, group.card.dispenser.project_key))
            if fast_raise:
                raise ValueError(message)

    return error_groups

from app.saint.helpers import *
from app.saint.assets import *
import re
class St_MDB:
    def cluster_resolve(self, data, id_type=None):

        support_types = ('clusterId', 'subclusterId', 'shardId', 'fqdn', 'instanceId')
        headers = {
            'X-YaCloud-SubjectToken': self.iam_token,
            'content-type': 'application/json'
        }

        if id_type == 'fqdn':
            if 'mdb.yandexcloud.net' not in data:
                data = '{host}.mdb.yandexcloud.net'.format(host=data)

        payload = {
            id_type: data
        }
        if id_type is None or id_type not in support_types:
            logging.error('Unknown type received')
            return None

        r = requests.get(self.endpoints.mdb_search, headers=headers, data=json.dumps(payload))
        res = r.json()
        logging.debug(res)
        if r.status_code != 200:
            print('Unable to resolve this cluster type yet.')
            logging.error(res.get('message'))
            return None

        if re.search('managed-mongodb', str(res)):
            return self.mongo_parse(res)
        if re.search('managed-mysql', str(res)):
            return self.mysql_parse(res)
        if re.search('managed-postgresql', str(res)):
            return self.pgsql_parse(res)
        if re.search('managed-redis', str(res)):
            return self.redis_parse(res)
        if re.search('managed-clickhouse', str(res)):
            return self.clickhouse_parse(res)
        if re.search('hadoop', str(res)):
            return self.hadoop_parse(res)
        if re.search('managed-kafka', str(res)):
            return self.kafka_parse(res)

        logging.error('No clusters found or unknown error. Contact @the-nans')

    def mongo_parse(self, json_data):
        # Base info
        result = {
            'type': 'managed-mongodb',
            'cluster_id': json_data.get('id'),
            'cloud_id': json_data.get('cloudId'),
            'folder_id': json_data.get('folderId'),
            'name': json_data.get('name'),
            'env': json_data.get('environment'),
            'health': json_data.get('health'),
            'status': json_data.get('status'),
            'created_at': json_data.get('createdAt'),
            'net_id': json_data.get('networkId'),
            'sgs_id': ", ".join(json_data.get('securityGroupIds')) if json_data.get(
                'securityGroupIds') else self.format_sg_by_network_id(json_data.get('networkId'))
            # 'sharded': json_data.get('sharded')
        }

        # Resources and version

        for key in json_data.get('config').keys():
            if key.startswith('mongodb'):
                m_ver = key
        if not m_ver:
            logging.error('Error in parse API. Please contact @the-nans')
            return None

        try:
            result['preset'] = json_data['config'][m_ver]['mongod']['resources'].get('resourcePresetId')
            result['disk_type'] = json_data['config'][m_ver]['mongod']['resources'].get('diskTypeId')
            result['disk_size'] = get_bytes_size_string(
                int(json_data['config'][m_ver]['mongod']['resources'].get('diskSize')))
            result['version'] = json_data['config'].get('version')
        except (KeyError, TypeError, UnboundLocalError):
            logging.error('Error in parse API. Please contact @the-nans')
            return None

        try:
            host = json_data.get('hosts')[0]['name']
        except (KeyError, TypeError):
            host = None
        result['usage_link'] = get_nda_link(self.profile.solomon_cluster_usage_url.format(json_data.get('id'), host))

        hosts = []
        try:
            for host in json_data.get('hosts'):
                final = {
                    'name': host.get('name'),
                    'instance_id': host.get('instanceId'),
                    'role': host.get('role'),
                    'health': host.get('health'),
                    'public': host.get('assignPublicIp')
                }
                hosts.append(final)
        except (TypeError, AttributeError):
            pass  # fix it later

        result['hosts'] = hosts

        if self.args.db_config:
            pass

        return result

    def mysql_parse(self, json_data):
        # Base info
        result = {
            'type': 'managed-mysql',
            'cluster_id': json_data.get('id'),
            'cloud_id': json_data.get('cloudId'),
            'folder_id': json_data.get('folderId'),
            'name': json_data.get('name'),
            'env': json_data.get('environment'),
            'health': json_data.get('health'),
            'status': json_data.get('status'),
            'created_at': json_data.get('createdAt'),
            'net_id': json_data.get('networkId'),
            'sgs_id': ", ".join(json_data.get('securityGroupIds')) if json_data.get(
                'securityGroupIds') else self.format_sg_by_network_id(json_data.get('networkId'))
        }

        # Resources and version
        result['preset'] = json_data['config']['resources'].get('resourcePresetId')
        result['disk_type'] = json_data['config']['resources'].get('diskTypeId')
        result['disk_size'] = get_bytes_size_string(int(json_data['config']['resources'].get('diskSize')))
        result['version'] = json_data['config'].get('version')

        try:
            host = json_data.get('hosts')[0]['name']
        except (KeyError, TypeError):
            host = None
        result['usage_link'] = get_nda_link(self.profile.solomon_cluster_usage_url.format(json_data.get('id'), host))

        hosts = []
        try:
            for host in json_data.get('hosts'):
                final = {
                    'name': host.get('name'),
                    'instance_id': host.get('instanceId'),
                    'role': host.get('role'),
                    'health': host.get('health'),
                    'public': host.get('assignPublicIp')
                }
                hosts.append(final)
        except (TypeError, AttributeError):
            pass  # fix it later

        result['hosts'] = hosts

        if self.args.db_config:
            pass

        return result

    def pgsql_parse(self, json_data):
        # Base info
        result = {
            'type': 'managed-postgresql',
            'cluster_id': json_data.get('id'),
            'cloud_id': json_data.get('cloudId'),
            'folder_id': json_data.get('folderId'),
            'name': json_data.get('name'),
            'env': json_data.get('environment'),
            'health': json_data.get('health'),
            'status': json_data.get('status'),
            'created_at': json_data.get('createdAt'),
            'net_id': json_data.get('networkId'),
            'sgs_id': ", ".join(json_data.get('securityGroupIds')) if json_data.get(
                'securityGroupIds') else self.format_sg_by_network_id(json_data.get('networkId'))
        }

        # Resources and version
        result['preset'] = json_data['config']['resources'].get('resourcePresetId')
        result['disk_type'] = json_data['config']['resources'].get('diskTypeId')
        result['disk_size'] = get_bytes_size_string(int(json_data['config']['resources'].get('diskSize')))
        result['version'] = json_data['config'].get('version')

        try:
            host = json_data.get('hosts')[0]['name']
        except (KeyError, TypeError):
            host = None
        result['usage_link'] = get_nda_link(self.profile.solomon_cluster_usage_url.format(json_data.get('id'), host))

        hosts = []
        try:
            for host in json_data.get('hosts'):
                final = {
                    'name': host.get('name'),
                    'instance_id': host.get('instanceId'),
                    'role': host.get('role'),
                    'health': host.get('health'),
                    'public': host.get('assignPublicIp')
                }
                hosts.append(final)
        except (TypeError, AttributeError):
            pass  # fix it later

        result['hosts'] = hosts

        if self.args.db_config:
            pass

        return result

    def hadoop_parse(self, json_data):
        # Base info
        result = {
            'type': 'dataproc',
            'cluster_id': json_data.get('id'),
            'cloud_id': json_data.get('cloudId'),
            'folder_id': json_data.get('folderId'),
            'name': json_data.get('name'),
            'env': json_data.get('environment'),
            'health': json_data.get('health'),
            'status': json_data.get('status'),
            'created_at': json_data.get('createdAt'),
            'net_id': json_data.get('networkId'),
            'sgs_id': ", ".join(json_data.get('securityGroupIds')) if json_data.get(
                'securityGroupIds') else self.format_sg_by_network_id(json_data.get('networkId'))
            # 'sharded': json_data.get('sharded')
        }

        # Resources and version
        # result['preset'] = json_data['config']['hadoop']['resources'].get('resourcePresetId')
        # result['disk_type'] = json_data['config']['hadoop']['resources'].get('diskTypeId')
        # result['disk_size'] = get_bytes_size_string(int(json_data['config']['hadoop']['resources'].get('diskSize')))
        result['version'] = json_data['config'].get('versionId')
        result['services'] = ", ".join(json_data['config'].get('hadoop').get('services'))
        result['bucket'] = json_data.get('bucket')

        try:
            host = json_data.get('hosts')[0]['name']
        except (KeyError, TypeError):
            host = None
        result['usage_link'] = get_nda_link(self.profile.solomon_cluster_usage_url.format(json_data.get('id'), host))

        hosts = []
        try:
            for host in json_data.get('hosts'):
                final = {
                    'name': host.get('name'),
                    'instance_id': host.get('instanceId'),
                    'role': host.get('role'),
                    'health': host.get('health'),
                    'public': host.get('assignPublicIp')
                }
                hosts.append(final)
        except (TypeError, AttributeError):
            pass  # fix it later

        result['hosts'] = hosts

        if self.args.db_config:
            pass

        return result

    def kafka_parse(self, json_data):
        headers = {
            'Authorization': 'Bearer {}'.format(self.iam_token)
        }
        r = requests.get(
            self.endpoints.mdb_url + 'managed-kafka/v1/clusters/{clusterId}'.format(
                clusterId=json_data.get('id')),
            headers=headers, verify=False
        )
        res = r.json()
        if r.status_code != 200:
            logging.error(res.get('message'))

        # Base info
        result = {'type': 'managed-kafka',
                  'cluster_id': json_data.get('id'),
                  'cloud_id': self.get_folder_by_id(json_data.get('folderId')).id, #.get('cloudId'),
                  'folder_id': json_data.get('folderId'),
                  'name': json_data.get('name'),
                  'env': json_data.get('environment'),
                  'health': json_data.get('health'),
                  'status': json_data.get('status'),
                  'created_at': json_data.get('createdAt'),
                  'net_id': json_data.get('networkId'),
                  'sgs_id': ", ".join(json_data.get('securityGroupIds')) if json_data.get(
                      'securityGroupIds') else self.format_sg_by_network_id(json_data.get('networkId'))
                  # 'sharded': json_data.get('sharded')
                  }

        result['preset'] = res['config']['kafka']['resources'].get('resourcePresetId')
        result['disk_type'] = res['config']['kafka']['resources'].get('diskTypeId')
        result['disk_size'] = get_bytes_size_string(int(res['config']['kafka']['resources'].get('diskSize')))
        result['version'] = res['config'].get('version')

        r_hosts = requests.get(
            self.endpoints.mdb_url + 'managed-kafka/v1/clusters/{clusterId}/hosts'.format(
                clusterId=json_data.get('id')),
            headers=headers, verify=False
        )

        json_hosts = r_hosts.json()
        try:
            host = json_hosts.get('hosts')[0]['name']
        except (KeyError, TypeError):
            host = None
        result['usage_link'] = get_nda_link(self.profile.solomon_cluster_usage_url.format(json_hosts.get('id'), host))

        hosts = []
        try:
            for host in json_hosts.get('hosts'):
                final = {
                    'name': host.get('name'),
                    'instance_id': 'None',
                    'role': host.get('role'),
                    'health': host.get('health'),
                    'public': host.get('assignPublicIp')
                }
                hosts.append(final)
        except (TypeError, AttributeError):
            pass  # fix it later

        result['hosts'] = hosts

        if self.args.db_config:
            pass

        return result

    def clickhouse_parse(self, json_data):
        # Base info
        result = {
            'type': 'managed-clickhouse',
            'cluster_id': json_data.get('id'),
            'cloud_id': json_data.get('cloudId'),
            'folder_id': json_data.get('folderId'),
            'name': json_data.get('name'),
            'env': json_data.get('environment'),
            'health': json_data.get('health'),
            'status': json_data.get('status'),
            'created_at': json_data.get('createdAt'),
            'net_id': json_data.get('networkId'),
            'sgs_id': ", ".join(json_data.get('securityGroupIds')) if json_data.get(
                'securityGroupIds') else self.format_sg_by_network_id(json_data.get('networkId'))
            # 'sharded': json_data.get('sharded')
        }

        # Resources and version
        result['preset'] = json_data['config']['clickhouse']['resources'].get('resourcePresetId')
        result['disk_type'] = json_data['config']['clickhouse']['resources'].get('diskTypeId')
        result['disk_size'] = get_bytes_size_string(
            int(json_data['config']['clickhouse']['resources'].get('diskSize')))
        result['version'] = json_data['config'].get('version')

        try:
            host = json_data.get('hosts')[0]['name']
        except (KeyError, TypeError):
            host = None
        result['usage_link'] = get_nda_link(self.profile.solomon_cluster_usage_url.format(json_data.get('id'), host))

        hosts = []
        try:
            for host in json_data.get('hosts'):
                final = {
                    'name': host.get('name'),
                    'instance_id': host.get('instanceId'),
                    'role': host.get('type'),
                    'health': host.get('health'),
                    'public': host.get('assignPublicIp')
                }
                hosts.append(final)
        except (TypeError, AttributeError):
            pass  # fix it later

        result['hosts'] = hosts

        if self.args.db_config:
            pass

        return result

    def redis_parse(self, json_data):
        # Base info
        result = {'type': 'managed-redis', 'cluster_id': json_data.get('id'), 'cloud_id': json_data.get('cloudId'),
                  'folder_id': json_data.get('folderId'), 'name': json_data.get('name'),
                  'env': json_data.get('environment'), 'health': json_data.get('health'),
                  'status': json_data.get('status'), 'created_at': json_data.get('createdAt'),
                  'net_id': json_data.get('networkId'),
                  'sgs_id': ", ".join(json_data.get('securityGroupIds')) if json_data.get(
                      'securityGroupIds') else self.format_sg_by_network_id(json_data.get('networkId')),
                  'preset': json_data['config']['resources'].get('resourcePresetId'),
                  'disk_type': json_data['config']['resources'].get('diskTypeId'),
                  'disk_size': get_bytes_size_string(int(json_data['config']['resources'].get('diskSize'))),
                  'version': json_data['config'].get('version')}

        # Resources and version

        try:
            host = json_data.get('hosts')[0]['name']
        except (KeyError, TypeError):
            host = None
        result['usage_link'] = get_nda_link(self.profile.solomon_cluster_usage_url.format(json_data.get('id'), host))

        hosts = []
        try:
            for host in json_data.get('hosts'):
                final = {
                    'name': host.get('name'),
                    'instance_id': host.get('instanceId'),
                    'role': host.get('role'),
                    'health': host.get('health'),
                    'public': host.get('assignPublicIp') if host.get('assignPublicIp') else 'UNKNOWN'
                }
                hosts.append(final)
        except (TypeError, AttributeError):
            pass  # fix it later

        result['hosts'] = hosts

        if self.args.db_config:
            pass

        return result

    def get_all_clusters(self, cloud_id):
        headers = {
            'Authorization': 'Bearer {}'.format(self.iam_token)
        }

        final = []
        if self.folders_get_by_cloud(cloud_id):
            for folder in self.folders_get_by_cloud(cloud_id).folders:
                folder_id = folder.id
                for dbtype, dbvalue in DB_TYPES.items():
                    if dbtype == 'hadoop':
                        r = requests.get(
                            self.endpoints.mdb_url.replace('mdb', 'dataproc',
                                                           1) + '{type}/v1/clusters?folderId={folder}'.format(type=dbvalue,
                                                                                                              folder=folder_id),
                            headers=headers, verify=False
                        )

                    else:
                        r = requests.get(
                            self.endpoints.mdb_url + '{type}/v1/clusters?folderId={folder}'.format(type=dbvalue,
                                                                                                   folder=folder_id),
                            headers=headers, verify=False
                        )
                    res = r.json()
                    logging.debug(res)

                    if r.status_code != 200:
                        logging.error(res.get('message'))

                    else:
                        for cluster in res.get('clusters', {}):
                            if dbtype == 'mysql':
                                final.append(self.mysql_parse(cluster))
                            elif dbtype == 'mongo':
                                final.append(self.mongo_parse(cluster))
                            elif dbtype == 'ch':
                                final.append(self.clickhouse_parse(cluster))
                            elif dbtype == 'pg':
                                final.append(self.pgsql_parse(cluster))
                            elif dbtype == 'redis':
                                final.append(self.redis_parse(cluster))
                            elif dbtype == 'hadoop':
                                final.append(self.hadoop_parse(cluster))
                            elif dbtype == 'kafka':
                                final.append(self.kafka_parse(cluster))
            #                 # ============== TMP! ==================
            #                 else:
            #                     final.append(cluster)

            # c=1             # ============== /TMP ==================
        return final

    def format_sg_by_network_id(self, network_id):

        network=self.get_network(network_id)
        result = {
            'id': network.id,
            'name': network.name,
            'description': network.description,
            'sg': network.default_security_group_id
        }

        return result.get('sg') + " (default)" if result.get('sg') else None

    def cluster_operation_get(self, cluster_id, db_type=None):

        headers = {
            'Authorization': 'Bearer {}'.format(self.iam_token),
            'content-type': 'application/json'
        }
        link = '{type}/v1/clusters/{cluster}/operations'.format(type=db_type, cluster=cluster_id)
        if db_type == 'dataproc':
            r = requests.get(self.endpoints.mdb_url.replace('mdb', 'dataproc', 1) + link, headers=headers, verify=False)

        else:
            r = requests.get(self.endpoints.mdb_url + link, headers=headers, verify=False)

        ops = r.json()
        logging.debug(ops)

        final = []
        for op in ops.get('operations', []):
            if op.get('done'):
                op_status = 'DONE'
            else:
                op_status = 'RUNNING'
            result = {
                'operation_id': op.get('id'),
                'description': op.get('description'),
                'created_at': op.get('createdAt')[:-8:],
                'modified_at': op.get('modifiedAt')[:-8:],
                'created_by': op.get('createdBy'),
                'done': 'ERROR: {}'.format(op.get('error').get('message')) if op.get('error') else op_status,
                'error': 'ERR' if op.get('error') else 'None'
            }
            final.append(result)
        return final

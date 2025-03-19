pillar = {
    'zone_ids': ['ru-central1-a', 'ru-central1-b', 'ru-central1-c'],
    'network_id': 'network_id_1',
    'user_subnet_ids': ['user_subnet_ids_1', 'user_subnet_ids_2', 'user_subnet_ids_3'],
    'service_account_id': 'service_account_id_1',
    'min_servers_per_zone': 1,
    'kubernetes_cluster_id': 'kubernetes_cluster_id_1',
    'postgresql_cluster_id': 'postgresql_cluster_id_1',
    'node_service_account_id': 'node_service_account_id_1',
    'postgresql_hostname': 'postgresql_hostname_1',
    'service_subnet_ids': ['service_subnet_id_1'],
    'db_name': 'db_name_1',
    'db_user_name': 'db_user_name_1',
    'db_user_password': {
        "data": "3bpyOML1kND3D4C1R_9uSHkcgC7T6LGglzSJEr_RB7AslFPm9reZtOKg9THu15hEHZQ-d9t9RzGMsMld",
        "encryption_version": 1,
    },
    'kubernetes_namespace_name': 'namespace_1',
}


def get_state_with_pillar(state):
    state['metadb']['queries'] = [{'query': 'get_pillar', 'result': [{'value': pillar}]}] + state['metadb']['queries']
    return state

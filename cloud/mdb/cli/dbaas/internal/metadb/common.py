def to_db_cluster_type(cluster_type):
    return None if not cluster_type else cluster_type + '_cluster'


def to_db_cluster_types(cluster_types):
    if not cluster_types:
        return cluster_types

    return [to_db_cluster_type(cluster_type) for cluster_type in cluster_types]


def to_cluster_type(db_cluster_type):
    if db_cluster_type is None:
        return None
    return db_cluster_type.replace('_cluster', '')

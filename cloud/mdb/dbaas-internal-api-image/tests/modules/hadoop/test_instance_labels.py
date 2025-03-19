'''
Test for Hadoop instances lables
'''
from unittest.mock import Mock, patch

from dbaas_internal_api.modules.hadoop.utils import extend_instance_labels

subclusters = [
    {
        'cid': 'cid1',
        'role': 'hadoop_cluster.masternode',
        'subcid': 'subcid1',
    },
]

g = Mock()
g.folder = {'folder_ext_id': 'folder_id1'}
g.cloud = {'cloud_ext_id': 'cloud_id1'}


@patch('dbaas_internal_api.modules.hadoop.utils.g', g)
def test_correct_extend_with_new_labels():
    user_labels = {'foo': 'bar'}
    result = {
        'subcid1': {
            'folder_id': 'folder_id1',
            'cloud_id': 'cloud_id1',
            'cluster_id': 'cid1',
            'subcluster_id': 'subcid1',
            'subcluster_role': 'masternode',
            'foo': 'bar',
        },
    }
    assert extend_instance_labels(subclusters, user_labels) == result


@patch('dbaas_internal_api.modules.hadoop.utils.g', g)
def test_correct_extend_with_update_old_key():
    user_labels = {'my_label': 'my_updated_label'}
    result = {
        'subcid1': {
            'folder_id': 'folder_id1',
            'cloud_id': 'cloud_id1',
            'cluster_id': 'cid1',
            'subcluster_id': 'subcid1',
            'subcluster_role': 'masternode',
            'my_label': 'my_updated_label',
        },
    }
    assert extend_instance_labels(subclusters, user_labels) == result


@patch('dbaas_internal_api.modules.hadoop.utils.g', g)
def test_correct_extend_with_multiple_labels():
    user_labels = {'spam': '7', 'ololo': 'ololo'}
    result = {
        'subcid1': {
            'folder_id': 'folder_id1',
            'cloud_id': 'cloud_id1',
            'cluster_id': 'cid1',
            'subcluster_id': 'subcid1',
            'subcluster_role': 'masternode',
            'spam': '7',
            'ololo': 'ololo',
        },
    }
    assert extend_instance_labels(subclusters, user_labels) == result


@patch('dbaas_internal_api.modules.hadoop.utils.g', g)
def test_correct_extend_with_empty_labels():
    user_labels = {'': ''}
    result = {
        'subcid1': {
            'folder_id': 'folder_id1',
            'cloud_id': 'cloud_id1',
            'cluster_id': 'cid1',
            'subcluster_id': 'subcid1',
            'subcluster_role': 'masternode',
            '': '',
        },
    }
    assert extend_instance_labels(subclusters, user_labels) == result


@patch('dbaas_internal_api.modules.hadoop.utils.g', g)
def test_user_override_id_label():
    user_labels = {
        'cluster_id': 'user_want_to_set_this_clusterid',
        'subcluster_id': 'user_want_to_set_this_subclusterid',
        'subcluster_role': 'user_want_to_set_this_subcluster_role',
    }
    result = {
        'subcid1': {
            'folder_id': 'folder_id1',
            'cloud_id': 'cloud_id1',
            'cluster_id': 'user_want_to_set_this_clusterid',
            'subcluster_id': 'user_want_to_set_this_subclusterid',
            'subcluster_role': 'user_want_to_set_this_subcluster_role',
        },
    }
    assert extend_instance_labels(subclusters, user_labels) == result


@patch('dbaas_internal_api.modules.hadoop.utils.g', g)
def test_correct_extend_to_empty_labels_dict():
    user_labels = {
        '1': '2',
        'cluster_id': 'user_want_to_set_this_clusterid',
        'subcluster_id': 'user_want_to_set_this_subclusterid',
        'subcluster_role': 'user_want_to_set_this_subcluster_role',
    }
    result = {
        'subcid1': {
            'folder_id': 'folder_id1',
            'cloud_id': 'cloud_id1',
            'cluster_id': 'user_want_to_set_this_clusterid',
            'subcluster_id': 'user_want_to_set_this_subclusterid',
            'subcluster_role': 'user_want_to_set_this_subcluster_role',
            '1': '2',
        },
    }
    assert extend_instance_labels(subclusters, user_labels) == result


@patch('dbaas_internal_api.modules.hadoop.utils.g', g)
def test_correct_extend_with_multiple_subclusters():
    user_labels = {'foo': 'bar'}

    subclusters_with_multiple_subclusters = [
        {
            'cid': 'cid1',
            'subcid': 'subcid1',
            'role': 'hadoop_cluster.masternode',
        },
        {
            'cid': 'cid1',
            'subcid': 'subcid2',
            'role': 'hadoop_cluster.datanode',
        },
    ]
    result_with_multiple_subclusters = {
        'subcid1': {
            'folder_id': 'folder_id1',
            'cloud_id': 'cloud_id1',
            'cluster_id': 'cid1',
            'subcluster_id': 'subcid1',
            'subcluster_role': 'masternode',
            'foo': 'bar',
        },
        'subcid2': {
            'folder_id': 'folder_id1',
            'cloud_id': 'cloud_id1',
            'cluster_id': 'cid1',
            'subcluster_id': 'subcid2',
            'subcluster_role': 'datanode',
            'foo': 'bar',
        },
    }
    assert (
        extend_instance_labels(subclusters_with_multiple_subclusters, user_labels) == result_with_multiple_subclusters
    )

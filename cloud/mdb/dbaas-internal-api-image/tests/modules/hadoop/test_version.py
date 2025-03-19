from datetime import datetime

import pytest
import semver
from flask import Flask
from flask_appconfig import AppConfig


from dbaas_internal_api.modules.hadoop.pillar import HadoopPillar
from dbaas_internal_api.modules.hadoop.constants import (
    DATAPROC_INIT_ACTIONS_VERSION,
    DATAPROC_LIGHTWEIGHT_SPARK_VERSION,
    DATAPROC_LIGHTWEIGHT_HIVE_VERSION,
    MASTER_SUBCLUSTER_TYPE,
    COMPUTE_SUBCLUSTER_TYPE,
)
from dbaas_internal_api.modules.hadoop.traits import ClusterService
from dbaas_internal_api.modules.hadoop.version import (
    compare_version_strings,
    get_dataproc_images_config,
    get_latest_dataproc_version_by_prefix,
)
from dbaas_internal_api.modules.hadoop.validation import (
    validate_services,
    validate_initialization_actions,
    validate_subclusters_and_services,
)

from dbaas_internal_api.core.exceptions import DbaasClientError
from cloud.mdb.internal.python.compute import images


compute_images = [
    images.ImageModel(
        name='dataproc-image-1-0-trunk-1-0-0',
        id='image_id_1.0.1',
        description='Image 1.0',
        folder_id='folder1',
        min_disk_size=21474836480,
        status=images.ImageStatus.READY,
        created_at=datetime.now(),
        labels={'version': '1.0.1'},
        family='',
    ),
    images.ImageModel(
        name='',
        id='image_id_1.3.1',
        description='Image 1.3',
        folder_id='folder1',
        min_disk_size=21474836480,
        status=images.ImageStatus.READY,
        created_at=datetime.now(),
        labels={'version': '1.3.1'},
        family='dataproc-image-1-3',
    ),
    images.ImageModel(
        name='',
        id='image_id_1.4.1',
        description='Image 1',
        folder_id='folder1',
        min_disk_size=21474836480,
        status=images.ImageStatus.READY,
        created_at=datetime.now(),
        labels={'version': '1.4.1'},
        family='dataproc-image-1-4',
    ),
    images.ImageModel(
        id='image_id_2.0.0',
        name='',
        description='Image 2',
        folder_id='folder1',
        min_disk_size=21474836480,
        status=images.ImageStatus.READY,
        created_at=datetime.now(),
        labels={'version': '2.0.0'},
        family='yandex-dataproc-image-2-0',
    ),
    images.ImageModel(
        id='image_id_2.0.1',
        name='',
        description='Image 2',
        folder_id='folder1',
        min_disk_size=21474836480,
        status=images.ImageStatus.READY,
        created_at=datetime.now(),
        labels={'version': '2.0.1'},
        family='yandex-dataproc-image-2-0',
    ),
    images.ImageModel(
        id='image_id_2.0.11',
        name='',
        description='Image 2',
        folder_id='folder1',
        min_disk_size=21474836480,
        status=images.ImageStatus.READY,
        created_at=datetime.now(),
        labels={'version': '2.0.11'},
        family='yandex-dataproc-image-2-0',
    ),
    images.ImageModel(
        id='image_id_99.0.0',
        name='',
        description='Image 99',
        folder_id='folder1',
        min_disk_size=21474836480,
        status=images.ImageStatus.READY,
        created_at=datetime.now(),
        labels={'version': '99.0.0'},
        family='yandex-dataproc-image-99-0',
    ),
    images.ImageModel(
        id='image_id_2.0.11',
        name='',
        description='Image 2',
        folder_id='folder1',
        min_disk_size=21474836480,
        status=images.ImageStatus.READY,
        created_at=datetime.now(),
        labels={'version': '2.0.12'},
        family='wrong_family',
    ),
    images.ImageModel(
        id='image_id_2.0.39',
        name='',
        description='Image with lightweight spark',
        folder_id='folder1',
        min_disk_size=21474836480,
        status=images.ImageStatus.READY,
        created_at=datetime.now(),
        labels={'version': str(DATAPROC_LIGHTWEIGHT_SPARK_VERSION)},
        family='yandex-dataproc-image-2-0',
    ),
    images.ImageModel(
        id='image_id_2.0.40',
        name='',
        description='Image with init actions',
        folder_id='folder1',
        min_disk_size=21474836480,
        status=images.ImageStatus.READY,
        created_at=datetime.now(),
        labels={'version': str(DATAPROC_INIT_ACTIONS_VERSION)},
        family='yandex-dataproc-image-2-0',
    ),
    images.ImageModel(
        id='image_id_2.0.44',
        name='',
        description='Image with lightweight hive metastore',
        folder_id='folder1',
        min_disk_size=21474836480,
        status=images.ImageStatus.READY,
        created_at=datetime.now(),
        labels={'version': str(DATAPROC_LIGHTWEIGHT_HIVE_VERSION)},
        family='yandex-dataproc-image-2-0',
    ),
]
HADOOP_IMAGES = {
    '2.0.0': {
        'name': '2.0.0',
        'version': '2.0.0',
        'description': 'image with monitoring',
        'imageMinSize': 16106127360,
        'supported_pillar': '1.1.0',
        'services': {
            'hdfs': {
                'version': '2.8.5',
                'default': True,
            },
            'yarn': {
                'version': '2.8.5',
                'default': True,
            },
            'mapreduce': {
                'version': '2.8.5',
                'default': True,
                'deps': ['yarn'],
            },
            'tez': {
                'version': '0.9.1',
                'default': True,
                'deps': ['yarn'],
            },
            'zookeeper': {
                'version': '3.4.6',
                'default': True,
            },
            'hbase': {
                'version': '1.3.3',
                'default': True,
                'deps': ['zookeeper', 'hdfs', 'yarn'],
            },
            'hive': {
                'version': '2.3.4',
                'default': True,
            },
            'sqoop': {
                'version': '1.4.6',
                'default': True,
            },
            'flume': {
                'version': '1.8.0',
                'default': True,
            },
            'spark': {
                'version': '2.2.1',
                'default': True,
                'deps': ['yarn'],
            },
            'zeppelin': {
                'version': '0.7.3',
                'default': True,
            },
            'oozie': {
                'version': '4.3.1',
                'default': False,
                'deps': ['zookeeper'],
            },
            'livy': {
                'version': '0.7.0',
                'default': False,
                'deps': ['spark'],
            },
        },
        'roles_services': {
            'hadoop_cluster.masternode': [
                'hdfs',
                'yarn',
                'mapreduce',
                'zookeeper',
                'hbase',
                'hive',
                'sqoop',
                'spark',
                'zeppelin',
                'oozie',
                'livy',
            ],
            'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark', 'flume'],
            'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez', 'flume'],
        },
    },
    '1.4.0': {
        'default': True,
        'name': '1.4.0',
        'version': '1.4.0',
        'description': 'image with monitoring',
        'imageMinSize': 16106127360,
        'supported_pillar': '1.1.0',
        'services': {
            'hdfs': {
                'version': '2.8.5',
                'default': True,
            },
            'yarn': {
                'version': '2.8.5',
                'default': True,
                'deps': ['hdfs'],
            },
            'mapreduce': {
                'version': '2.8.5',
                'default': True,
                'deps': ['yarn'],
            },
            'tez': {
                'version': '0.9.1',
                'default': True,
                'deps': ['yarn'],
            },
            'zookeeper': {
                'version': '3.4.6',
                'default': True,
            },
            'hbase': {
                'version': '1.3.3',
                'default': True,
                'deps': ['zookeeper', 'hdfs', 'yarn'],
            },
            'hive': {
                'version': '2.3.4',
                'default': True,
                'deps': ['yarn'],
            },
            'sqoop': {
                'version': '1.4.6',
                'default': True,
            },
            'flume': {
                'version': '1.8.0',
                'default': True,
            },
            'spark': {
                'version': '2.2.1',
                'default': True,
                'deps': ['yarn', 'hdfs'],
            },
            'zeppelin': {
                'version': '0.7.3',
                'default': True,
            },
            'oozie': {
                'version': '4.3.1',
                'default': False,
                'deps': ['zookeeper'],
            },
            'livy': {
                'version': '0.7.0',
                'default': False,
                'deps': ['spark'],
            },
        },
        'roles_services': {
            'hadoop_cluster.masternode': [
                'hdfs',
                'yarn',
                'mapreduce',
                'zookeeper',
                'hbase',
                'hive',
                'sqoop',
                'spark',
                'zeppelin',
                'oozie',
                'livy',
            ],
            'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark', 'flume'],
            'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez', 'flume'],
        },
    },
    '1.3.0': {
        'name': '1.3.0',
        'version': '1.3.0',
        'deprecated': True,
        'allow_deprecated_feature_flag': 'MDB_DATAPROC_ALLOW_DEPRECATED_VERSIONS',
        'description': 'image with apache livy',
        'imageMinSize': 16106127360,
        'supported_pillar': '1.1.0',
        'services': {
            'hdfs': {
                'version': '2.8.5',
                'default': True,
            },
            'yarn': {
                'version': '2.8.5',
                'default': True,
                'deps': ['hdfs'],
            },
            'mapreduce': {
                'version': '2.8.5',
                'default': True,
                'deps': ['yarn'],
            },
            'tez': {
                'version': '0.9.1',
                'default': True,
                'deps': ['yarn'],
            },
            'zookeeper': {
                'version': '3.4.6',
                'default': True,
            },
            'hbase': {
                'version': '1.3.3',
                'default': True,
                'deps': ['zookeeper', 'hdfs', 'yarn'],
            },
            'hive': {
                'version': '2.3.4',
                'default': True,
                'deps': ['yarn'],
            },
            'sqoop': {
                'version': '1.4.6',
                'default': True,
            },
            'flume': {
                'version': '1.8.0',
                'default': True,
            },
            'spark': {
                'version': '2.2.1',
                'default': True,
                'deps': ['yarn', 'hdfs'],
            },
            'zeppelin': {
                'version': '0.7.3',
                'default': True,
            },
            'oozie': {
                'version': '4.3.1',
                'default': False,
                'deps': ['zookeeper'],
            },
            'livy': {
                'version': '0.7.0',
                'default': False,
                'deps': ['spark'],
            },
        },
        'roles_services': {
            'hadoop_cluster.masternode': [
                'hdfs',
                'yarn',
                'mapreduce',
                'zookeeper',
                'hbase',
                'hive',
                'sqoop',
                'spark',
                'zeppelin',
                'oozie',
                'livy',
            ],
            'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark', 'flume'],
            'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez', 'flume'],
        },
    },
    '1.0.0': {
        'name': '1.0.0',
        'version': '1.0.0',
        'deprecated': True,
        'allow_deprecated_feature_flag': 'MDB_DATAPROC_ALLOW_DEPRECATED_VERSIONS',
        'description': 'image with apache livy',
        'imageMinSize': 16106127360,
        'supported_pillar': '1.1.0',
        'services': {
            'hdfs': {
                'version': '2.8.5',
                'default': True,
            },
            'yarn': {
                'version': '2.8.5',
                'default': True,
            },
            'mapreduce': {
                'version': '2.8.5',
                'default': True,
                'deps': ['yarn'],
            },
            'tez': {
                'version': '0.9.1',
                'default': True,
                'deps': ['yarn'],
            },
            'zookeeper': {
                'version': '3.4.6',
                'default': True,
            },
            'hbase': {
                'version': '1.3.3',
                'default': True,
                'deps': ['zookeeper', 'hdfs', 'yarn'],
            },
            'hive': {
                'version': '2.3.4',
                'default': True,
                'deps': ['yarn'],
            },
            'sqoop': {
                'version': '1.4.6',
                'default': True,
            },
            'flume': {
                'version': '1.8.0',
                'default': True,
            },
            'spark': {
                'version': '2.2.1',
                'default': True,
                'deps': ['yarn'],
            },
            'zeppelin': {
                'version': '0.7.3',
                'default': True,
            },
            'oozie': {
                'version': '4.3.1',
                'default': False,
                'deps': ['zookeeper'],
            },
            'livy': {
                'version': '0.7.0',
                'default': False,
                'deps': ['spark'],
            },
        },
        'roles_services': {
            'hadoop_cluster.masternode': [
                'hdfs',
                'yarn',
                'mapreduce',
                'zookeeper',
                'hbase',
                'hive',
                'sqoop',
                'spark',
                'zeppelin',
                'oozie',
                'livy',
            ],
            'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark', 'flume'],
            'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez', 'flume'],
        },
    },
    '99.0.0': {
        'name': '99.0.0',
        'version': '99.0.0',
        'feature_flag': 'MDB_DATAPROC_IMAGE_99',
        'description': 'image with monitoring',
        'imageMinSize': 16106127360,
        'supported_pillar': '1.1.0',
        'services': {
            'hdfs': {
                'version': '2.8.5',
                'default': True,
            },
            'yarn': {
                'version': '2.8.5',
                'default': True,
                'deps': ['hdfs'],
            },
            'mapreduce': {
                'version': '2.8.5',
                'default': True,
                'deps': ['yarn'],
            },
            'tez': {
                'version': '0.9.1',
                'default': True,
                'deps': ['yarn'],
            },
            'zookeeper': {
                'version': '3.4.6',
                'default': True,
            },
            'hbase': {
                'version': '1.3.3',
                'default': True,
                'deps': ['zookeeper', 'hdfs', 'yarn'],
            },
            'hive': {
                'version': '2.3.4',
                'default': True,
                'deps': ['yarn'],
            },
            'sqoop': {
                'version': '1.4.6',
                'default': True,
            },
            'flume': {
                'version': '1.8.0',
                'default': True,
            },
            'spark': {
                'version': '2.2.1',
                'default': True,
                'deps': ['yarn', 'hdfs'],
            },
            'zeppelin': {
                'version': '0.7.3',
                'default': True,
            },
            'oozie': {
                'version': '4.3.1',
                'default': False,
                'deps': ['zookeeper'],
            },
            'livy': {
                'version': '0.7.0',
                'default': False,
                'deps': ['spark'],
            },
        },
        'roles_services': {
            'hadoop_cluster.masternode': [
                'hdfs',
                'yarn',
                'mapreduce',
                'zookeeper',
                'hbase',
                'hive',
                'sqoop',
                'spark',
                'zeppelin',
                'oozie',
                'livy',
            ],
            'hadoop_cluster.datanode': ['hdfs', 'yarn', 'mapreduce', 'tez', 'hbase', 'spark', 'flume'],
            'hadoop_cluster.computenode': ['yarn', 'mapreduce', 'tez', 'flume'],
        },
    },
}


dataproc_images_config = get_dataproc_images_config(with_deprecated=True, images_config=HADOOP_IMAGES)


def test_get_latest_dataproc_version_by_prefix():
    image_version, image_id, image_config = get_latest_dataproc_version_by_prefix(
        version_prefix='',
        compute_images=compute_images,
        dataproc_images_config=dataproc_images_config,
    )
    assert image_version == semver.VersionInfo(99, 0, 0)


def test_get_latest_dataproc_version_by_prefix_10():
    image_version, image_id, image_config = get_latest_dataproc_version_by_prefix(
        version_prefix='1.0',
        compute_images=compute_images,
        dataproc_images_config=dataproc_images_config,
    )
    assert image_version == semver.VersionInfo(1, 0, 1)


def test_get_latest_dataproc_version_by_prefix_2():
    image_version, _, _ = get_latest_dataproc_version_by_prefix(
        version_prefix='2',
        compute_images=compute_images,
        dataproc_images_config=dataproc_images_config,
    )
    assert image_version == semver.VersionInfo(2, 0, 44)

    image_version, _, _ = get_latest_dataproc_version_by_prefix(
        version_prefix='2.0',
        compute_images=compute_images,
        dataproc_images_config=dataproc_images_config,
    )
    assert image_version == semver.VersionInfo(2, 0, 44)


def test_get_latest_dataproc_version_by_prefix_200():
    image_version, image_id, image_config = get_latest_dataproc_version_by_prefix(
        version_prefix='2.0.0',
        compute_images=compute_images,
        dataproc_images_config=dataproc_images_config,
    )
    assert image_version == semver.VersionInfo(2, 0, 0)


def test_get_latest_dataproc_version_by_prefix_201():
    image_version, image_id, image_config = get_latest_dataproc_version_by_prefix(
        version_prefix='2.0.1',
        compute_images=compute_images,
        dataproc_images_config=dataproc_images_config,
    )
    assert image_version == semver.VersionInfo(2, 0, 1)


def test_get_latest_dataproc_version_by_prefix_1():
    image_version, image_id, image_config = get_latest_dataproc_version_by_prefix(
        version_prefix='1',
        compute_images=compute_images,
        dataproc_images_config=dataproc_images_config,
    )
    assert image_version == semver.VersionInfo(1, 4, 1)


def test_get_latest_dataproc_version_by_prefix_13():
    image_version, _, _ = get_latest_dataproc_version_by_prefix(
        version_prefix='1.3',
        compute_images=compute_images,
        dataproc_images_config=dataproc_images_config,
    )
    assert image_version == semver.VersionInfo(1, 3, 1)


def test_get_latest_dataproc_version_by_prefix_4():
    with pytest.raises(DbaasClientError):
        get_latest_dataproc_version_by_prefix(
            version_prefix='4',
            compute_images=compute_images,
            dataproc_images_config=dataproc_images_config,
        )


def test_compare_version_strings():
    assert compare_version_strings('1.0.0', '1.0.0') == 0
    assert compare_version_strings('1.0.0', '1.0.1') == -1
    assert compare_version_strings('1.0.1', '1.0.0') == 1
    assert compare_version_strings('1.0.1', '1.0') == -1
    assert compare_version_strings('1.0', '1.0.1') == 1
    assert compare_version_strings('2.0', '1.0.1') == 1
    assert compare_version_strings('1.0.1', '2.0') == -1
    assert compare_version_strings('1.0', '2.0.1') == -1
    assert compare_version_strings('2.0.1', '1.0') == 1
    assert compare_version_strings('2.0', '2.0.1') == 1
    assert compare_version_strings('2.0.1', '2.0') == -1
    assert compare_version_strings('1.2.0', '1.11.0') == -1
    assert compare_version_strings('1.11.0', '1.2.0') == 1


def test_versions_with_init_actions():
    expected_exception = 'To specify initialization actions dataproc image version must be at least `2.0.40`'
    with pytest.raises(DbaasClientError) as excinfo:
        validate_initialization_actions(version='1.4.0')
    assert expected_exception in str(excinfo.value)

    # Check correct versions
    validate_initialization_actions(version='2.0.40')


@pytest.fixture
def app_config():
    """
    Allows testing code to user current_app.config
    """
    app = Flask("dbaas_internal_api")
    AppConfig(app, None)
    app.config['HADOOP_IMAGES'] = HADOOP_IMAGES

    with app.app_context():
        yield app.config


def test_lightweight_spark_fails_before_39(app_config):
    image_version, _, image_config = get_latest_dataproc_version_by_prefix(
        version_prefix='2.0.1',
        compute_images=compute_images,
        dataproc_images_config=dataproc_images_config,
    )
    pillar = HadoopPillar.make(image_config=image_config)
    pillar.set_subcluster(
        {
            'name': 'driver',
            'subcid': 'subcid1',
            'role': MASTER_SUBCLUSTER_TYPE,
            'services': [ClusterService.yarn, ClusterService.spark],
        }
    )
    pillar.set_subcluster(
        {'name': 'executor', 'subcid': 'subcid3', 'role': COMPUTE_SUBCLUSTER_TYPE, 'services': [ClusterService.yarn]}
    )
    pillar.services = [ClusterService.yarn, ClusterService.spark]

    validate_subclusters_and_services(pillar)
    threshold = str(DATAPROC_LIGHTWEIGHT_SPARK_VERSION)
    expected_exception = (
        f"422 Unprocessable Entity: To use service `spark` without `hdfs` "
        f"dataproc image version must be at least `{threshold}`"
    )
    with pytest.raises(DbaasClientError) as excinfo:
        validate_services(pillar.services, image_config, image_version)
    assert expected_exception in str(excinfo.value)


def test_lightweight_spark_works_since_39(app_config):
    image_version, _, image_config = get_latest_dataproc_version_by_prefix(
        version_prefix=str(DATAPROC_LIGHTWEIGHT_SPARK_VERSION),
        compute_images=compute_images,
        dataproc_images_config=dataproc_images_config,
    )
    pillar = HadoopPillar.make(image_config=image_config)
    pillar.set_subcluster(
        {
            'name': 'driver',
            'subcid': 'subcid1',
            'role': MASTER_SUBCLUSTER_TYPE,
            'services': [ClusterService.yarn, ClusterService.spark],
        }
    )
    pillar.set_subcluster(
        {'name': 'executor', 'subcid': 'subcid3', 'role': COMPUTE_SUBCLUSTER_TYPE, 'services': [ClusterService.yarn]}
    )
    pillar.services = [ClusterService.yarn, ClusterService.spark]

    validate_subclusters_and_services(pillar)
    validate_services(pillar.services, image_config, image_version)


def test_lightweight_hive_fails_before_44(app_config):
    image_version, _, image_config = get_latest_dataproc_version_by_prefix(
        version_prefix='2.0.39',
        compute_images=compute_images,
        dataproc_images_config=dataproc_images_config,
    )
    pillar = HadoopPillar.make(image_config=image_config)
    pillar.set_subcluster(
        {
            'name': 'main',
            'subcid': 'subcid1',
            'role': MASTER_SUBCLUSTER_TYPE,
            'services': [ClusterService.hive],
        }
    )
    pillar.services = [ClusterService.hive]

    validate_subclusters_and_services(pillar)
    threshold = str(DATAPROC_LIGHTWEIGHT_HIVE_VERSION)
    expected_exception = (
        f"422 Unprocessable Entity: To use service `hive` without `yarn` "
        f"dataproc image version must be at least `{threshold}`"
    )
    with pytest.raises(DbaasClientError) as excinfo:
        validate_services(pillar.services, image_config, image_version)
    assert expected_exception in str(excinfo.value)


def test_lightweight_hive_works_after_44(app_config):
    image_version, _, image_config = get_latest_dataproc_version_by_prefix(
        version_prefix='2.0.44',
        compute_images=compute_images,
        dataproc_images_config=dataproc_images_config,
    )
    pillar = HadoopPillar.make(image_config=image_config)
    pillar.set_subcluster(
        {
            'name': 'main',
            'subcid': 'subcid1',
            'role': MASTER_SUBCLUSTER_TYPE,
            'services': [ClusterService.hive],
        }
    )
    pillar.services = [ClusterService.hive]

    validate_subclusters_and_services(pillar)
    validate_services(pillar.services, image_config, image_version)

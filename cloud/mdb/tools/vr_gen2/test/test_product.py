from typing import List

import yaml
from cloud.mdb.tools.vr_gen2.internal.data_sources import (
    FlavorSourceBase,
    DiskTypeIdSourceBase,
    GeoIdSource,
)
from cloud.mdb.tools.vr_gen2.internal.models import ResourceDefSchema, ClusterTypes
from cloud.mdb.tools.vr_gen2.internal.models.base import (
    ListDiskOptionsDef,
    ValidResourceDef,
    FlavorType,
    HostCount,
    DiskSize,
    Segment,
)
from cloud.mdb.tools.vr_gen2.internal.models.clickhouse import ClickhouseClusterDef
from cloud.mdb.tools.vr_gen2.internal.models.kafka import KafkaClusterDef
from cloud.mdb.tools.vr_gen2.internal.models.resource import ResourceDumper
from cloud.mdb.tools.vr_gen2.internal.product import generate_resources


class FlavorSource(FlavorSourceBase):
    def filter_ids_by_type(self, pattern) -> List[str]:
        return ['flavor-test-1']

    def filter_ids_by_name(self, pattern) -> List[str]:
        return ['flavor-test']


class DiskIdSource(DiskTypeIdSourceBase):
    def get_by_ext_id(self, ext_id) -> int:
        return 1050


def test_produced_data():
    schema = ResourceDefSchema(
        cluster_types=ClusterTypes(
            clickhouse_cluster=ClickhouseClusterDef(
                zk=ListDiskOptionsDef(
                    gp2=[
                        ValidResourceDef(
                            flavor_types=[FlavorType(name="s1.nano")],
                            host_count=[HostCount(max=2)],
                            feature_flags=[],
                            disk_sizes=[DiskSize(int8range=Segment(1, 4), custom_range=None, custom_sizes=None)],
                            default_disk_size='2',
                            excluded_geo=[],
                        )
                    ]
                )
            ),
            kafka_cluster=KafkaClusterDef(
                zk=ListDiskOptionsDef(
                    gp2=[
                        ValidResourceDef(
                            flavor_types=[FlavorType(name="s1.nano")],
                            host_count=[HostCount(max=2)],
                            feature_flags=[],
                            disk_sizes=[DiskSize(int8range=Segment(1, 4), custom_range=None, custom_sizes=None)],
                            default_disk_size=None,
                            excluded_geo=[],
                        )
                    ]
                )
            ),
        )
    )

    result = generate_resources(
        schema,
        geo_source=GeoIdSource(
            """
        - name: aws-frankfurt
          geo_id: 9090
        """
        ),
        flavor_source=FlavorSource(),
        disk_source=DiskIdSource(),
    )
    resources_yaml = yaml.dump(data=result, Dumper=ResourceDumper)

    assert (
        """- cluster_type: clickhouse_cluster
  default_disk_size: 2
  disk_size_range:
    int8range:
    - 1
    - 4
  disk_sizes: null
  disk_type_id: 1050
  feature_flag: null
  flavor: flavor-test
  geo_id: 9090
  id: 1
  max_hosts: 2
  min_hosts: 1
  role: zk
- cluster_type: kafka_cluster
  default_disk_size: null
  disk_size_range:
    int8range:
    - 1
    - 4
  disk_sizes: null
  disk_type_id: 1050
  feature_flag: null
  flavor: flavor-test
  geo_id: 9090
  id: 2
  max_hosts: 2
  min_hosts: 1
  role: zk
"""
        == resources_yaml
    )


def test_produced_data_with_feature_flags():
    schema = ResourceDefSchema(
        cluster_types=ClusterTypes(
            clickhouse_cluster=ClickhouseClusterDef(
                zk=ListDiskOptionsDef(
                    gp2=[
                        ValidResourceDef(
                            flavor_types=[FlavorType(name="s1.nano")],
                            host_count=[HostCount(max=2)],
                            feature_flags=["FLAG1", "FLAG2"],
                            disk_sizes=[DiskSize(int8range=Segment(1, 4), custom_range=None, custom_sizes=None)],
                            default_disk_size='2',
                            excluded_geo=[],
                        )
                    ]
                )
            ),
            kafka_cluster=KafkaClusterDef(
                zk=ListDiskOptionsDef(
                    gp2=[
                        ValidResourceDef(
                            flavor_types=[FlavorType(name="s1.nano")],
                            host_count=[HostCount(max=2)],
                            feature_flags=["FLAG1", "FLAG2"],
                            disk_sizes=[DiskSize(int8range=Segment(1, 4), custom_range=None, custom_sizes=None)],
                            default_disk_size=None,
                            excluded_geo=[],
                        )
                    ]
                )
            ),
        )
    )

    result = generate_resources(
        schema,
        geo_source=GeoIdSource(
            """
            - name: aws-frankfurt
              geo_id: 9090
            """
        ),
        flavor_source=FlavorSource(),
        disk_source=DiskIdSource(),
    )
    resources_yaml = yaml.dump(data=result, Dumper=ResourceDumper)

    assert (
        """- cluster_type: clickhouse_cluster
  default_disk_size: 2
  disk_size_range:
    int8range:
    - 1
    - 4
  disk_sizes: null
  disk_type_id: 1050
  feature_flag: FLAG1
  flavor: flavor-test
  geo_id: 9090
  id: 1
  max_hosts: 2
  min_hosts: 1
  role: zk
- cluster_type: clickhouse_cluster
  default_disk_size: 2
  disk_size_range:
    int8range:
    - 1
    - 4
  disk_sizes: null
  disk_type_id: 1050
  feature_flag: FLAG2
  flavor: flavor-test
  geo_id: 9090
  id: 2
  max_hosts: 2
  min_hosts: 1
  role: zk
- cluster_type: kafka_cluster
  default_disk_size: null
  disk_size_range:
    int8range:
    - 1
    - 4
  disk_sizes: null
  disk_type_id: 1050
  feature_flag: FLAG1
  flavor: flavor-test
  geo_id: 9090
  id: 3
  max_hosts: 2
  min_hosts: 1
  role: zk
- cluster_type: kafka_cluster
  default_disk_size: null
  disk_size_range:
    int8range:
    - 1
    - 4
  disk_sizes: null
  disk_type_id: 1050
  feature_flag: FLAG2
  flavor: flavor-test
  geo_id: 9090
  id: 4
  max_hosts: 2
  min_hosts: 1
  role: zk
"""
        == resources_yaml
    )


def test_produced_data_with_geo_filter():
    schema = ResourceDefSchema(
        cluster_types=ClusterTypes(
            clickhouse_cluster=ClickhouseClusterDef(
                zk=ListDiskOptionsDef(
                    gp2=[
                        ValidResourceDef(
                            flavor_types=[FlavorType(name="s1.nano")],
                            host_count=[HostCount(max=2)],
                            feature_flags=[],
                            disk_sizes=[DiskSize(int8range=Segment(1, 4), custom_range=None, custom_sizes=None)],
                            default_disk_size='2',
                            excluded_geo=['incomplete_ch_geo'],
                        )
                    ]
                )
            ),
            kafka_cluster=KafkaClusterDef(
                zk=ListDiskOptionsDef(
                    gp2=[
                        ValidResourceDef(
                            flavor_types=[FlavorType(name="s1.nano")],
                            host_count=[HostCount(max=2)],
                            feature_flags=[],
                            disk_sizes=[DiskSize(int8range=Segment(1, 4), custom_range=None, custom_sizes=None)],
                            default_disk_size=None,
                            excluded_geo=['incomplete_kafka_geo'],
                        )
                    ]
                )
            ),
        )
    )

    result = generate_resources(
        schema,
        geo_source=GeoIdSource(
            """
        - name: incomplete_ch_geo
          geo_id: 1
        - name: incomplete_kafka_geo
          geo_id: 2
        """
        ),
        flavor_source=FlavorSource(),
        disk_source=DiskIdSource(),
    )
    resources_yaml = yaml.dump(data=result, Dumper=ResourceDumper)

    assert (
        """- cluster_type: clickhouse_cluster
  default_disk_size: 2
  disk_size_range:
    int8range:
    - 1
    - 4
  disk_sizes: null
  disk_type_id: 1050
  feature_flag: null
  flavor: flavor-test
  geo_id: 2
  id: 1
  max_hosts: 2
  min_hosts: 1
  role: zk
- cluster_type: kafka_cluster
  default_disk_size: null
  disk_size_range:
    int8range:
    - 1
    - 4
  disk_sizes: null
  disk_type_id: 1050
  feature_flag: null
  flavor: flavor-test
  geo_id: 1
  id: 2
  max_hosts: 2
  min_hosts: 1
  role: zk
"""
        == resources_yaml
    )

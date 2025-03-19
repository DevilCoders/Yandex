{% set cluster_map = grains['cluster_map'] %}
{% set environment = cluster_map.environment %}

resources_sets:
  default:
    clouds:
    - mdb
    - yandexcloud
    - ycloud-platform
    - ycmarketplace
    - yc-e2e-tests
    folders:
    - public
    - standard-images
    - service-images
    images:
    - ubuntu1604_ci_image
    - ubuntu1604_base_image
    - image_ci
    - image_e2e
    - image_fedora_27
    - image_trusty
    - image_xenial
    - image_centos_6_vanila
    - image_centos_7_vanila
    - image_ubuntu_trusty_vanila
    - image_ubuntu_xenial_vanila
    - image_fedora_28_vanila
    - image_windows_2012_vanila
    - mdb_image_ci
    - marketplace_image_ci
    - ycp_image_ci
    instance_groups:
    - mdb_slb-adapter-svm
    - marketplace_slb-adapter-svm
    - ycp_slb-adapter-svm
    placement_groups:
{% for placement_group in cluster_map.get('placement_groups', {}).keys() %}
    - {{ placement_group }}
{% endfor %}
    pooling:
    {%- set zones = salt['grains.get']('cluster_map:availability_zones', []) -%}
    {%- for zone_id in zones %}
        - pooling_centos_6_vanila_{{ zone_id }}
        - pooling_e2e_{{ zone_id }}
    {%- endfor %}
    user_data:
    - user_yctest
    - user_yc_testing
    - user_yndx-cloud-mkt
  service_vm:
    clouds:
    - mdb
    - yandexcloud
    - ycloud-platform
    - ycmarketplace
    - yc-e2e-tests
    images:
    - image_ci
    - base_image
    - ubuntu1604_ci_image
    - ubuntu1604_base_image
    instance_groups:
{% for instance_group in cluster_map.instance_per_nodes %}
    - {{ instance_group }}
{% endfor %}
    placement_groups:
{% for placement_group in cluster_map.get('placement_groups', {}).keys() %}
    - {{ placement_group }}
{% endfor %}
    quotas:
    - nbs_disks_yandexcloud
    - instances_yandexcloud
    - cores_yandexcloud
    - memory_yandexcloud
    - snapshots_yandexcloud
    - images_yandexcloud
    - total_disk_size_yandexcloud
    - network_hdd_total_disk_size_yandexcloud
    - network_ssd_total_disk_size_yandexcloud
    - total_snapshot_size_yandexcloud
    - networks_yandexcloud
    - subnets_yandexcloud
    - external_addresses_yandexcloud
    - external_static_addresses_yandexcloud
    - cores_e2e
    - instances_e2e
    - memory_e2e
    - networks_e2e
    - subnets_e2e
    - disks_e2e
    - total_disk_size_e2e
    - network_hdd_total_disk_size_e2e
    - network_ssd_total_disk_size_e2e
  hwlab_default_resources:  # CLOUD-14461: do not provision unneeded images
    clouds:
    - mdb
    - yandexcloud
    - ycloud-platform
    - ycmarketplace
    - yc-e2e-tests
    folders:
    - public
    - standard-images
    - service-images
    images:
    - image_ci
    - image_e2e
    - image_centos_6_vanila
    - ubuntu1604_ci_image
    - ubuntu1604_base_image
    instance_groups:
    - mdb_slb-adapter-svm
    - marketplace_slb-adapter-svm
    - ycp_slb-adapter-svm
    placement_groups:
{% for placement_group in cluster_map.get('placement_groups', {}).keys() %}
    - {{ placement_group }}
{% endfor %}
    pooling:
    {%- set zones = salt['grains.get']('cluster_map:availability_zones', []) -%}
    {%- for zone_id in zones %}
        - pooling_centos_6_vanila_{{ zone_id }}
        - pooling_e2e_{{ zone_id }}
    {%- endfor %}
    user_data:
    - user_yctest
    - user_yc_testing
  cloudvm_init:
    clouds:
    - yandexcloud
    folders:
    - dogfood
  cloudvm:
    clouds:
    - yandexcloud
    images:
    - image_cirros
    quotas:
      - nbs_disks_yandexcloud
      - instances_yandexcloud
      - cores_yandexcloud
      - memory_yandexcloud
      - images_yandexcloud
      - total_disk_size_yandexcloud
      - network_hdd_total_disk_size_yandexcloud
      - network_ssd_total_disk_size_yandexcloud
      - total_snapshot_size_yandexcloud
      - networks_yandexcloud
      - subnets_yandexcloud
      - external_addresses_yandexcloud
      - external_static_addresses_yandexcloud
  prod:
    clouds:
    - mdb
    - yandexcloud
    - ycloud-platform
    - ycmarketplace
{% if environment == 'prod' %}
    - yc-yql-api
{% endif %}
    - yc-e2e-tests
    folders:
    - public
    - standard-images
    images:
    - image_centos_6_vanila
    - image_centos_7_vanila
    - image_ubuntu_trusty_vanila
    - image_ubuntu_xenial_vanila
    - image_fedora_28_vanila
    - image_windows_2012_vanila
    - image_ci
    - image_e2e
    - mdb_image_ci
    - marketplace_image_ci
    - ycp_image_ci
{% if environment == 'prod' %}
    - yqlapi_image_ci
{% endif %}
    instance_groups:
    - mdb_slb-adapter-svm
    - marketplace_slb-adapter-svm
    - ycp_slb-adapter-svm
{% if environment == 'prod' %}
    - yqlapi_slb-adapter-svm
{% endif %}
    pooling:
    {%- set zones = salt['grains.get']('cluster_map:availability_zones', []) -%}
    {%- for zone_id in zones %}
        - pooling_centos_6_vanila_{{ zone_id }}
        - pooling_centos_7_vanila_{{ zone_id }}
        - pooling_ubuntu_trusty_vanila_{{ zone_id }}
        - pooling_ubuntu_xenial_vanila_{{ zone_id }}
        - pooling_fedora_28_vanila_{{ zone_id }}
        - pooling_windows_2012_vanila_{{ zone_id }}
        - pooling_e2e_{{ zone_id }}
    {%- endfor %}
    user_data:
    - user_yctest
    - user_yc_testing
    - user_yndx-cloud-mkt
  prod_service_vm:
    clouds:
    - mdb
    - yandexcloud
    - ycloud-platform
    - ycmarketplace
{% if environment == 'prod' %}
    - yc-yql-api
{% endif %}
    images:
    - image_ci
    - base_image
    instance_groups:
{% for instance_group in cluster_map.instance_per_nodes %}
    - {{ instance_group }}
{% endfor %}
    placement_groups:
{% for placement_group in cluster_map.get('placement_groups', {}).keys() %}
    - {{ placement_group }}
{% endfor %}
    quotas:
    - nbs_disks_yandexcloud
    - instances_yandexcloud
    - cores_yandexcloud
    - memory_yandexcloud
    - snapshots_yandexcloud
    - images_yandexcloud
    - total_disk_size_yandexcloud
    - total_snapshot_size_yandexcloud
    - networks_yandexcloud
    - subnets_yandexcloud
    - external_addresses_yandexcloud
    - external_static_addresses_yandexcloud
    - nbs_disks_mkt
    - instances_mkt
    - cores_mkt
    - memory_mkt
    - snapshots_mkt
    - images_mkt
    - total_disk_size_tb_mkt
    - total_snapshot_size_mkt
    - networks_mkt
    - subnets_mkt
    - external_addresses_mkt
    - external_static_addresses_mkt
    - nbs_disks_mdb
    - instances_mdb
    - cores_mdb
    - memory_mdb
    - snapshots_mdb
    - images_mdb
    - total_disk_size_tb_mdb
    - total_snapshot_size_mdb
    - networks_mdb
    - subnets_mdb
    - external_addresses_mdb
    - external_static_addresses_mdb
  empty: []

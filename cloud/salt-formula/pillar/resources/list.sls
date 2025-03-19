{% set cluster_map = grains['cluster_map'] %}
{% set environment = cluster_map.environment %}

{# since name -- isn't unique anymore, we using IDs for PROD/PREPROD to meet security requirements #}
{# see CLOUD-14086 for details #}

{% set public_folder = "public" %}
{% if environment == 'dev' %}
    {% set pooling_count = 1 %}
    {% set e2e_pooling_count = 5 %}
{% else %}
    {% set pooling_count = 5 %}
    {% set e2e_pooling_count = 15 %}
{% endif %}
{% set zones = salt['grains.get']('cluster_map:availability_zones', []) %}
{% set ci_base_url = "https://s3.mds.yandex.net/yc-bootstrap/ci-20190305-2200" %}
{% set cloud_base_url = "https://s3.mds.yandex.net/yc-bootstrap/cloud-base-20190517-0432" %}
{% set e2e_tests_url = "https://s3.mds.yandex.net/yc-bootstrap/e2e-tests-20190111-1128" %}
{% set cirros_url = "https://s3.mds.yandex.net/yc-bootstrap/cirros-0.3.4-x86_64-disk.img" %}
{% set ubuntu1604_ci_stable_url = "https://s3.mds.yandex.net/yc-bootstrap/ubuntu1604-ci-stable" %}
{% set ubuntu1604_base_stable_url = "https://s3.mds.yandex.net/yc-bootstrap/ubuntu1604-base-stable" %}

list_resources:
  clouds:
    yandexcloud:
      yc.cloud:
        # NOTE: policies are deprecated, but left here to prevent yaml parsing errors
        - policies: []

    ycmarketplace:
      yc.cloud:
        - policies: []

    mdb:
      yc.cloud:
        - policies: []

    ycloud-platform:
      yc.cloud:
        - policies: []

    yc-e2e-tests:
      yc.cloud:
        - policies: []

{% if environment == 'prod' %}
    yc-yql-api:
      yc.cloud:
        - policies: []
{% endif %}

  folders:
    {{ public_folder }}:
      yc.folder:
        - cloud: yandexcloud
        - public_roles:
          - compute.images.user
        - require:
          - yandexcloud
    standard-images:
      yc.folder:
        - id: standard-images
        - cloud: ycmarketplace
        - public_roles:
          - compute.images.user
        - require:
          - ycmarketplace
    service-images:
      yc.folder:
        - id: service-images
        - cloud: yandexcloud
        - require:
          - yandexcloud
    e2e:
      yc.folder:
        - cloud: yc-e2e-tests
        - require:
          - yc-e2e-tests
    dogfood:
      yc.folder:
        - cloud: yandexcloud
        - require:
          - yandexcloud

  images:
    ubuntu1604_ci_image:
      yc.image:
        - name: ubuntu1604-ci-stable
        - url_var: CI1604_IMG_URL
        - default_url: {{ ubuntu1604_ci_stable_url }}
        - cloud: yandexcloud
        - folder: service-images
        - description: {{ ubuntu1604_ci_stable_url }}
        - require:
          - yandexcloud

    ubuntu1604_base_image:
      yc.image:
        - name: ubuntu1604-base-stable
        - url_var: BASE1604_IMG_URL
        - default_url: {{ ubuntu1604_base_stable_url }}
        - cloud: yandexcloud
        - folder: service-images
        - description: {{ ubuntu1604_base_stable_url }}
        - require:
          - yandexcloud

    image_ci:
      yc.image:
        - name: ci-base
        - url_var: NEWCI_IMAGE_URL
        - default_url: {{ ci_base_url }}
        - cloud: yandexcloud
        - folder: dogfood
        - description: {{ ci_base_url }}
        - require:
          - yandexcloud

    base_image:
      yc.image:
        - name: cloud-base
        - url_var: BASE_IMAGE_URL
        - default_url: {{ cloud_base_url }}
        - cloud: yandexcloud
        - folder: dogfood
        - description: {{ cloud_base_url }}
        - require:
          - yandexcloud

    mdb_image_ci:
      yc.image:
        - name: ci-base
        - url_var: NEWCI_IMAGE_URL
        - default_url: {{ ci_base_url }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb

    marketplace_image_ci:
      yc.image:
        - name: ci-base
        - url_var: NEWCI_IMAGE_URL
        - default_url: {{ ci_base_url }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace

    ycp_image_ci:
      yc.image:
        - name: ci-base
        - url_var: NEWCI_IMAGE_URL
        - default_url: {{ ci_base_url }}
        - cloud: ycloud-platform
        - folder: common
        - require:
          - ycloud-platform

{% if environment == 'prod' %}
    yqlapi_image_ci:
      yc.image:
        - name: ci-base
        - url_var: NEWCI_IMAGE_URL
        - default_url: {{ ci_base_url }}
        - cloud: yc-yql-api
        - folder: yql-api-server
        - require:
          - yc-yql-api
{% endif %}

    image_cirros:
      yc.image:
        - name: cirros-0-3-4-x86-64
        - url_var: CIRROS_URL
        - default_url: {{ cirros_url }}
        - description: {{ cirros_url }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud

    image_e2e:
      yc.image:
        - name: e2e-tests
        - url_var: E2E_TESTS_URL
        - default_url: {{ e2e_tests_url }}
        - description: {{ e2e_tests_url }}
        - cloud: yc-e2e-tests
        - folder: e2e
        - require:
          - yc-e2e-tests

    image_centos_6_vanila:
      yc.image:
        - name: centos-6-vanila
        - url_var: IMAGE_URL_CENTOS_6_VANILA
        - default_url: https://s3.mds.yandex.net/yc-bootstrap/centos-6-20180322-1548
        - cloud: yandexcloud
        - folder: {{ public_folder }}
        - require:
          - yandexcloud

    image_centos_7_vanila:
      yc.image:
        - name: centos-7-vanila
        - url_var: IMAGE_URL_CENTOS_7_VANILA
        - default_url: https://s3.mds.yandex.net/yc-bootstrap/centos-7-20180322-1548
        - cloud: yandexcloud
        - folder: {{ public_folder }}
        - require:
          - yandexcloud

    image_ubuntu_trusty_vanila:
      yc.image:
        - name: ubuntu-trusty-vanila
        - url_var: IMAGE_URL_UBUNTU_TRUSTY_VANILA
        - default_url: https://s3.mds.yandex.net/yc-bootstrap/ubuntu-trusty-20180322-1548
        - cloud: yandexcloud
        - folder: {{ public_folder }}
        - require:
          - yandexcloud

    image_ubuntu_xenial_vanila:
      yc.image:
        - name: ubuntu-xenial-vanila
        - url_var: IMAGE_URL_UBUNTU_XENIAL_VANILA
        - default_url: https://s3.mds.yandex.net/yc-bootstrap/ubuntu-xenial-20180322-1548
        - cloud: yandexcloud
        - folder: {{ public_folder }}
        - require:
          - yandexcloud

    image_fedora_28_vanila:
      yc.image:
        - name: fedora-28-vanila
        - url_var: IMAGE_URL_FEDORA_28_VANILA
        - default_url: https://s3.mds.yandex.net/yc-bootstrap/linux-fedora-28-20180521-1420
        - cloud: yandexcloud
        - folder: {{ public_folder }}
        - require:
          - yandexcloud

    image_windows_2012_vanila:
      yc.image:
        - name: windows-2012-vanila
        - url_var: IMAGE_URL_WINDOWS_2012_VANILA
        - default_url: https://yc-bootstrap.s3.mds.yandex.net/windows_server_2012_r2_standard_eval_kvm_20170321.qcow2
        - cloud: yandexcloud
        - folder: {{ public_folder }}
        - require:
          - yandexcloud

    image_fedora_27:
      yc.image:
        - name: linux-fedora-27
        - url_var: FEDORA_27_URL
        - default_url: https://s3.mds.yandex.net/yc-bootstrap/linux-fedora-27-20180322-1548
        - cloud: yandexcloud
        - folder: {{ public_folder }}
        - require:
          - yandexcloud

    image_trusty:
      yc.image:
        - name: linux-ubuntu-trusty
        - url_var: UBUNTU_TRUSTY_URL
        - default_url: https://s3.mds.yandex.net/yc-bootstrap/linux-ubuntu-trusty-20180625-1657
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud

    image_xenial:
      yc.image:
        - name: linux-ubuntu-xenial
        - url_var: UBUNTU_XENIAL_URL
        - default_url: https://s3.mds.yandex.net/yc-bootstrap/linux-ubuntu-xenial-20180625-1645
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud

  instance_groups:
  {% for instance_group, svms_per_node in cluster_map.instance_per_nodes.iteritems() %}
    {{ instance_group }}:
      yc.instance_group:
        - name: {{ instance_group }}
        - max_instances_per_node: {{ svms_per_node }}
        - require:
          - yandexcloud
  {% endfor %}
    # CLOUD-10045: need slb-adapter-svm instance group for mdb and marketplace
    mdb_slb-adapter-svm:
      yc.instance_group:
        - name: slb-adapter-svm
        - max_instances_per_node: {{ cluster_map['instance_per_nodes']['slb-adapter-svm']|default(1) }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb

    marketplace_slb-adapter-svm:
      yc.instance_group:
        - name: slb-adapter-svm
        - max_instances_per_node: {{ cluster_map['instance_per_nodes']['slb-adapter-svm']|default(1) }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace

    ycp_slb-adapter-svm:
      yc.instance_group:
        - name: slb-adapter-svm
        - max_instances_per_node: {{ cluster_map['instance_per_nodes']['slb-adapter-svm']|default(1) }}
        - cloud: ycloud-platform
        - folder: common
        - require:
          - ycloud-platform

{% if environment == 'prod' %}
    yqlapi_slb-adapter-svm:
      yc.instance_group:
        - name: slb-adapter-svm
        - max_instances_per_node: {{ cluster_map['instance_per_nodes']['slb-adapter-svm']|default(1) }}
        - cloud: yc-yql-api
        - folder: yql-api-server
        - require:
          - yc-yql-api
{% endif %}

  placement_groups:
  {% for placement_group, placement_group_config in cluster_map.get('placement_groups', {}).iteritems() %}
    {{ placement_group }}:
      yc.placement_group:
        - name: {{ placement_group }}
        - max_instances_per_node: {{ placement_group_config.get('max_instances_per_node', 'null') }}
        - max_instances_per_fault_domain: {{ placement_group_config.get('max_instances_per_fault_domain', 'null') }}
        - strict: {{ placement_group_config['strict'] }}
        - require:
          - yandexcloud
  {% endfor %}

  pooling:
    {%- for zone_id in zones %}
    pooling_centos_6_vanila_{{ zone_id }}:
      yc.pooling:
        - image: centos-6-vanila
        - zone_id: {{ zone_id }}
        - count: {{ pooling_count }}
        - folder: {{ public_folder }}
        - require:
          - image_centos_6_vanila

    pooling_centos_7_vanila_{{ zone_id }}:
      yc.pooling:
        - image: centos-7-vanila
        - zone_id: {{ zone_id }}
        - count: {{ pooling_count }}
        - folder: {{ public_folder }}
        - require:
          - image_centos_7_vanila

    pooling_ubuntu_trusty_vanila_{{ zone_id }}:
      yc.pooling:
        - image: ubuntu-trusty-vanila
        - zone_id: {{ zone_id }}
        - count: {{ pooling_count }}
        - folder: {{ public_folder }}
        - require:
          - image_ubuntu_trusty_vanila

    pooling_ubuntu_xenial_vanila_{{ zone_id }}:
      yc.pooling:
        - image: ubuntu-xenial-vanila
        - zone_id: {{ zone_id }}
        - count: {{ pooling_count }}
        - folder: {{ public_folder }}
        - require:
          - image_ubuntu_xenial_vanila

    pooling_fedora_28_vanila_{{ zone_id }}:
      yc.pooling:
        - image: fedora-28-vanila
        - zone_id: {{ zone_id }}
        - count: {{ pooling_count }}
        - folder: {{ public_folder }}
        - require:
          - image_fedora_28_vanila

    pooling_windows_2012_vanila_{{ zone_id }}:
      yc.pooling:
        - image: windows-2012-vanila
        - zone_id: {{ zone_id }}
        - count: {{ pooling_count }}
        - folder: {{ public_folder }}
        - require:
          - image_windows_2012_vanila

    pooling_fedora_27_{{ zone_id }}:
      yc.pooling:
        - image: linux-fedora-27
        - zone_id: {{ zone_id }}
        - count: {{ pooling_count }}
        - folder: {{ public_folder }}
        - require:
          - image_fedora_27

    pooling_trusty_{{ zone_id }}:
      yc.pooling:
        - image: linux-ubuntu-trusty
        - zone_id: {{ zone_id }}
        - count: {{ pooling_count }}
        - folder: {{ public_folder }}
        - require:
          - image_trusty

    pooling_xenial_{{ zone_id }}:
      yc.pooling:
        - image: linux-ubuntu-xenial
        - zone_id: {{ zone_id }}
        - count: {{ pooling_count }}
        - folder: {{ public_folder }}
        - require:
          - image_xenial
    pooling_e2e_{{ zone_id }}:
      yc.pooling:
        - image: e2e-tests
        - cloud: yc-e2e-tests
        - folder: e2e
        - count: {{ e2e_pooling_count }}
        - zone_id: {{ zone_id }}
        - type_id: network-hdd
        - require:
          - image_e2e
    {% endfor %}

{%- set all_oct_clusters = cluster_map.oct.clusters %}
{%- set az_by_oct_cluster = {} %}
{%- for zone, clusters in cluster_map.oct_clusters_by_az | dictsort %}
  {%- for cluster in clusters -%}
    {%- do az_by_oct_cluster.update({cluster: zone}) -%}
  {% endfor %}
{%- endfor %}
  subnets:
    {%- for oct_cluster_id, oct_cluster in all_oct_clusters | dictsort %}
    {%- for net in oct_cluster.nets.manual|default([]) %}
    {%- set zone_id = az_by_oct_cluster[oct_cluster_id] %}
    {{ net.name }}-{{ zone_id }}:
      yc.subnet:
        - name: {{ net.name }}-{{ zone_id }}
        - network: {{ net.name }}
        - auto_create_network: true
        - zone_id: {{ zone_id }}
        {%- if net.get('v6') %}
        - v6_cidr_block: {{ net.v6 }}
        {%- endif %}
        {%- if net.get('v4') %}
        - v4_cidr_block: {{ net.v4 }}
        {%- endif %}

        {%- set export_rts = net.export_rts|list %}
        {%- set import_rts = net.import_rts|list %}
        {%- if not net.get('internal', False) %}
            {%- set export_rts = export_rts + oct_cluster.nets.upstream_rt|list %}
            {%- set import_rts = import_rts + oct_cluster.nets.downstream_rt|list %}
        {%- endif %}
        - extra_params:
            export_rts: {{ export_rts|yaml }}
            import_rts: {{ import_rts|yaml }}
            hbf_enabled: {{ net.hbf_enabled }}
            rpf_enabled: {{ net.rpf_enabled }}
        - cloud: {{ net.cloud }}
        - folder: {{ net.folder }}
        - require:
          - {{ net.cloud }}
    {%- endfor %}
    {%- endfor %}

  fip_buckets:
    {%- for oct_cluster_id, oct_cluster in all_oct_clusters | dictsort %}
    {%- set zone_id = az_by_oct_cluster[oct_cluster_id] %}

    # Public (aka white) FIPs
    {%- if 'external' in oct_cluster.nets %}
    fip-public-{{ zone_id }}:
      yc.fip_bucket:
        - name: public@{{ zone_id }}
        - flavor: public
        - scope: {{ zone_id }}
        - cidrs: {{ oct_cluster.nets.external['prefixes'] | yaml }}
        - import_rts: {{ oct_cluster.nets.external['downstream_rt'] | yaml }}
        - export_rts: {{ oct_cluster.nets.external['upstream_rt'] | yaml }}
    {%- endif %}

    # Martian FIPs (see CLOUD-8209)
    {%- if 'to_yandex' in oct_cluster.nets %}
    fip-martian-{{ zone_id }}:
      yc.fip_bucket:
        - name: martian@{{ zone_id }}
        - flavor: martian
        - scope: {{ zone_id }}
        - cidrs: {{ oct_cluster.nets.to_yandex['prefixes'] | yaml }}
        - import_rts: {{ oct_cluster.nets.to_yandex['downstream_rt'] | yaml }}
        - export_rts: {{ oct_cluster.nets.to_yandex['upstream_rt'] | yaml }}
    {%- endif %}

    # QRator (aka DDoS-protected) FIPs
    {%- if 'qrator' in oct_cluster.nets %}
    fip-qrator-{{ zone_id }}:
      yc.fip_bucket:
        - name: qrator@{{ zone_id }}
        - flavor: qrator
        - scope: {{ zone_id }}
        - cidrs: {{ oct_cluster.nets.qrator['prefixes'] | yaml }}
        - import_rts: {{ oct_cluster.nets.qrator['downstream_rt'] | yaml }}
        - export_rts: {{ oct_cluster.nets.qrator['upstream_rt'] | yaml }}
    {%- endif %}

    # SMTP direct (aka mail-enabled) FIPs
    {%- if 'smtp_direct' in oct_cluster.nets %}
    fip-smtp-direct-{{ zone_id }}:
      yc.fip_bucket:
        - name: smtp-direct@{{ zone_id }}
        - flavor: smtp-direct
        - scope: {{ zone_id }}
        - cidrs: {{ oct_cluster.nets.smtp_direct['prefixes'] | yaml }}
        - import_rts: {{ oct_cluster.nets.smtp_direct['downstream_rt'] | yaml }}
        - export_rts: {{ oct_cluster.nets.smtp_direct['upstream_rt'] | yaml }}
    {%- endif %}

    # Public ipv6 FIPs
    {%- if 'public_v6' in oct_cluster.nets %}
    fip-public-v6-{{ zone_id }}:
      yc.fip_bucket:
        - name: public@{{ zone_id }}
        - flavor: public
        - scope: {{ zone_id }}
        - cidrs: {{ oct_cluster.nets.public_v6['prefixes'] | yaml }}
        - import_rts: {{ oct_cluster.nets.public_v6['downstream_rt'] | yaml }}
        - export_rts: {{ oct_cluster.nets.public_v6['upstream_rt'] | yaml }}
        - ip_version: ipv6
    {%- endif %}

    # Yandex-only ipv6 FIPs
    {%- if 'yandex_only_v6' in oct_cluster.nets %}
    fip-yandex-only-v6-{{ zone_id }}:
      yc.fip_bucket:
        - name: yandex-only@{{ zone_id }}
        - flavor: yandex-only
        - scope: {{ zone_id }}
        - cidrs: {{ oct_cluster.nets.yandex_only_v6['prefixes'] | yaml }}
        - import_rts: {{ oct_cluster.nets.yandex_only_v6['downstream_rt'] | yaml }}
        - export_rts: {{ oct_cluster.nets.yandex_only_v6['upstream_rt'] | yaml }}
        - ip_version: ipv6
    {%- endif %}

    {%- endfor %}

  user_data:
    user_yctest:
      yc.user:
        - login: yctest
        - roles:
          - resource-manager.clouds.owner
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud

{# e2e user #}
    user_yc_testing:
      yc.user:
{%- if environment == 'prod' or environment == 'pre-prod' %}
        - login: yndx-yc-testing
{%- else %}
        - login: yndx-ycdf-testing
{%- endif %}
        - roles:
          - resource-manager.clouds.owner
          - internal.computeadmin
        - cloud: yc-e2e-tests
        - folder: e2e
        - require:
          - yc-e2e-tests

    user_yndx-cloud-mkt:
      yc.user:
        - login: yndx.cloud.mkt
        - roles:
          - resource-manager.clouds.owner
          - internal.computeadmin
        # NOTE: root roles allow actions in ALL clouds
        - root_roles:
          - internal.marketplaceagent
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace

{% for mdb_login in ('robot-mdb', 'robot-mdb-test') %}
    user_{{ mdb_login }}:
      yc.user:
        - login: {{ mdb_login }}
        - roles:
          - resource-manager.clouds.owner
          - internal.computeadmin
        # NOTE: root roles allow actions in ALL clouds
        - root_roles:
          - internal.mdbagent
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
{% endfor %}

  quotas:
{%- set number = 10000 %}
    nbs_disks_yandexcloud:
      yc.quota:
        - name: disk-count
        - limit: {{ number }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    instances_yandexcloud:
      yc.quota:
        - name: instance-count
        - limit: {{ number }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    snapshots_yandexcloud:
      yc.quota:
        - name: snapshot-count
        - limit: {{ number }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    images_yandexcloud:
      yc.quota:
        - name: image-count
        - limit: {{ number }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    total_disk_size_yandexcloud:
      yc.quota:
        - name: total-disk-size
        - limit: {{ number * 1099511627776 }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    network_hdd_total_disk_size_yandexcloud:
      yc.quota:
        - name: network-hdd-total-disk-size
        - limit: {{ number * 1099511627776 }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    network_ssd_total_disk_size_yandexcloud:
      yc.quota:
        - name: network-ssd-total-disk-size
        - limit: {{ number * 1099511627776 }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    total_snapshot_size_yandexcloud:
      yc.quota:
        - name: total-snapshot-size
        - limit: {{ number * 1099511627776 }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    cores_yandexcloud:
      yc.quota:
        - name: cores
        - limit: {{ number }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    memory_yandexcloud:
      yc.quota:
        - name: memory
        - limit: {{ number * 1099511627776 }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    networks_yandexcloud:
      yc.quota:
        - name: network-count
        - limit: {{ number }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    subnets_yandexcloud:
      yc.quota:
        - name: subnet-count
        - limit: {{ number }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    external_addresses_yandexcloud:
      yc.quota:
        - name: external-address-count
        - limit: {{ number }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud
    external_static_addresses_yandexcloud:
      yc.quota:
        - name: external-static-address-count
        - limit: {{ number }}
        - cloud: yandexcloud
        - folder: dogfood
        - require:
          - yandexcloud

    {# cloud for e2e test #}
{%- set e2e_cloud_q = 2000 %}
    cores_e2e:
      yc.quota:
        - name: cores
        - limit: {{ e2e_cloud_q }}
        - cloud: yc-e2e-tests
        - folder: e2e
        - require:
          - yc-e2e-tests
    instances_e2e:
      yc.quota:
        - name: instance-count
        - limit: {{ e2e_cloud_q }}
        - cloud: yc-e2e-tests
        - folder: e2e
        - require:
          - yc-e2e-tests
    memory_e2e:
      yc.quota:
        - name: memory
        - limit: {{ e2e_cloud_q * 1073741824 }}
        - cloud: yc-e2e-tests
        - folder: e2e
        - require:
          - yc-e2e-tests
    networks_e2e:
      yc.quota:
        - name: network-count
        - limit: {{ e2e_cloud_q }}
        - cloud: yc-e2e-tests
        - folder: e2e
        - require:
          - yc-e2e-tests
    subnets_e2e:
      yc.quota:
        - name: subnet-count
        - limit: {{ e2e_cloud_q }}
        - cloud: yc-e2e-tests
        - folder: e2e
        - require:
          - yc-e2e-tests
    disks_e2e:
      yc.quota:
        - name: disk-count
        - limit: {{ e2e_cloud_q }}
        - cloud: yc-e2e-tests
        - folder: e2e
        - require:
          - yc-e2e-tests
    total_disk_size_e2e:
      yc.quota:
        - name: total-disk-size
        - limit: {{ e2e_cloud_q * 1073741824 * 10 }}
        - cloud: yc-e2e-tests
        - folder: e2e
        - require:
          - yc-e2e-tests
    network_hdd_total_disk_size_e2e:
      yc.quota:
        - name: network-hdd-total-disk-size
        - limit: {{ e2e_cloud_q * 1073741824 * 10 }}
        - cloud: yc-e2e-tests
        - folder: e2e
        - require:
          - yc-e2e-tests
    network_ssd_total_disk_size_e2e:
      yc.quota:
        - name: network-ssd-total-disk-size
        - limit: {{ e2e_cloud_q * 1073741824 * 10 }}
        - cloud: yc-e2e-tests
        - folder: e2e
        - require:
          - yc-e2e-tests
    route_tables_e2e:
      yc.quota:
        - name: route-table-count
        - limit: 50
        - cloud: yc-e2e-tests
        - folder: e2e
        - require:
          - yc-e2e-tests
    {# end of e2e quotes #}

    nbs_disks_mkt:
      yc.quota:
        - name: disk-count
        - limit: {{ number }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    instances_mkt:
      yc.quota:
        - name: instance-count
        - limit: {{ number }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    snapshots_mkt:
      yc.quota:
        - name: snapshot-count
        - limit: {{ number }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    images_mkt:
      yc.quota:
        - name: image-count
        - limit: {{ number }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    total_disk_size_mkt:
      yc.quota:
        - name: total-disk-size
        - limit: {{ number * 1099511627776 }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    network_hdd_total_disk_size_mkt:
      yc.quota:
        - name: network-hdd-total-disk-size
        - limit: {{ number * 1099511627776 }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    network_ssd_total_disk_size_mkt:
      yc.quota:
        - name: network-ssd-total-disk-size
        - limit: {{ number * 1099511627776 }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    total_snapshot_size_mkt:
      yc.quota:
        - name: total-snapshot-size
        - limit: {{ number * 1099511627776 }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    cores_mkt:
      yc.quota:
        - name: cores
        - limit: {{ number }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    memory_mkt:
      yc.quota:
        - name: memory
        - limit: {{ number * 1099511627776 }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    networks_mkt:
      yc.quota:
        - name: network-count
        - limit: {{ number }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    subnets_mkt:
      yc.quota:
        - name: subnet-count
        - limit: {{ number }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    external_addresses_mkt:
      yc.quota:
        - name: external-address-count
        - limit: {{ number }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace
    external_static_addresses_mkt:
      yc.quota:
        - name: external-static-address-count
        - limit: {{ number }}
        - cloud: ycmarketplace
        - folder: default
        - require:
          - ycmarketplace

    nbs_disks_mdb:
      yc.quota:
        - name: disk-count
        - limit: {{ number }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    instances_mdb:
      yc.quota:
        - name: instance-count
        - limit: {{ number }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    snapshots_mdb:
      yc.quota:
        - name: snapshot-count
        - limit: {{ number }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    images_mdb:
      yc.quota:
        - name: image-count
        - limit: {{ number }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    total_disk_size_mdb:
      yc.quota:
        - name: total-disk-size
        - limit: {{ number * 1099511627776 }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    network_hdd_total_disk_size_mdb:
      yc.quota:
        - name: network-hdd-total-disk-size
        - limit: {{ number * 1099511627776 }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    network_ssd_total_disk_size_mdb:
      yc.quota:
        - name: network-ssd-total-disk-size
        - limit: {{ number * 1099511627776 }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    total_snapshot_size_mdb:
      yc.quota:
        - name: total-snapshot-size
        - limit: {{ number * 1099511627776 }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    cores_mdb:
      yc.quota:
        - name: cores
        - limit: {{ number }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    memory_mdb:
      yc.quota:
        - name: memory
        - limit: {{ number * 1099511627776 }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    networks_mdb:
      yc.quota:
        - name: network-count
        - limit: {{ number }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    subnets_mdb:
      yc.quota:
        - name: subnet-count
        - limit: {{ number }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    external_addresses_mdb:
      yc.quota:
        - name: external-address-count
        - limit: {{ number }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb
    external_static_address_mdb:
      yc.quota:
        - name: external-static-addresses-count
        - limit: {{ number }}
        - cloud: mdb
        - folder: control-plane
        - require:
          - mdb

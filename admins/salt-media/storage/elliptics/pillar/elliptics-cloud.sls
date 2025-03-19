cluster : elliptics-cloud

is_cloud: true

include:
  - units.ssl.cloud
  - units.ssl.nscfg
  - units.karl.control
  - units.federation
  - units.resource-provider

parsers-bin:
  - elliptics_client_parser.py
  - elliptics_server_parser.pl
  - mm-cache.py
  - couples_state.py

yasmagent:
  instance-getter:
    {% for itype in ['mastermindresizer', 'mdsnscfg'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    {% for itype in ['mdscloud', 'mdsmastermind', 'karl'] %}
    - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_cloud a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    {% for itype in ['storagesystem'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_cloud a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    - echo {{ grains['conductor']['fqdn'] }}:10010@mdscommon a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_mdscommon a_cgroup_elliptics-cloud

mastermind-namespace-space-lock-name: 'monrun-mastermind-namespace-space-production'
mastermind-namespace-space:
  ns_filter: '*'
  service: elliptics-cloud-mm-namespace-space
  log_level: DEBUG
  max_status: 2
  defaults:
    percent_warn: 4
    percent_crit: 2
    time_warn: 10d
    time_crit: 5d
    bytes_warn: 100G
    bytes_crit: 10G
  namespaces:
    default:
      percent_warn: 2
      percent_crit: 1
    disk:
      percent_warn: 4
      percent_crit: 3
    video-disk:
      time_warn: 2d
      time_crit: 1d
    repo:
      percent_warn: 3
      percent_crit: 2
    browser-tests:
      time_warn: 4d
      time_crit: 3d
    video-ya-events:
      percent_warn: 2
      percent_crit: 1
    turbo-commodity-feed:
      time_warn: 6d
      time_crit: 4d
    avatars-realty:
      percent_warn: 3
      percent_crit: 2
    s3-videohosting:
      percent_warn: 2
      percent_crit: 1
    avatars-socsnippet:
      percent_warn: 4
      percent_crit: 1
    matrix-router-result-testing:
      time_warn: 4d
      time_crit: 1d
      percent_warn: 4
      percent_crit: 2
    avatars-marketpictesting:
      percent_warn: 2
      percent_crit: 1

# FIXME
unit_duty_bot:
    storage:
        monrun: True
        actions: 
          - name: 'rotate-duty'
            params:
              - '--abc-sync'
          - name: 'duty-report'
            schedule: '20 11 * * 1,2,3,4,5'
          - name: 'calendar'
            schedule: '10 12 * * *'
            params:
              - '--abc-sync'
              - '--start `date +\%F -d "15 days"`'
              - '--end `date +\%F -d "15 days"`'
    storage-cloud:
        prefix: '-cloud'
        monrun: True
        actions: 
          - name: 'calendar'
            schedule: '20 12 * * *'
            params:
              - '--abc-sync'
              - '--start `date +\%F -d "15 days"`'
              - '--end `date +\%F -d "15 days"`'
          - name: 'abc-sync'
    storage-cloud-backup:
        prefix: '-cloud-backup'
        monrun: True
        actions: 
          - name: 'calendar'
            schedule: '25 12 * * *'
            params:
              - '--abc-sync'
              - '--start `date +\%F -d "15 days"`'
              - '--end `date +\%F -d "15 days"`'
          - name: 'abc-sync'
    storage-mds:
        prefix: '-mds'
        monrun: True
        actions: 
          - name: 'rotate-duty'
            params:
              - '--abc-sync'
          - name: 'calendar'
            schedule: '20 12 * * *'
            params:
              - '--abc-sync'
              - '--start `date +\%F -d "15 days"`'
              - '--end `date +\%F -d "15 days"`'
    storage-s3:
        prefix: '-s3'
        monrun: True
        actions: 
          - name: 'rotate-duty'
            params:
              - '--abc-sync'
          - name: 'calendar'
            schedule: '20 12 * * *'
            params:
              - '--abc-sync'
              - '--start `date +\%F -d "15 days"`'
              - '--end `date +\%F -d "15 days"`'
    storage-avatars:
        prefix: '-avatars'
        monrun: True
        actions: 
          - name: 'rotate-duty'
            params:
              - '--abc-sync'
          - name: 'calendar'
            schedule: '20 12 * * *'
            params:
              - '--abc-sync'
              - '--start `date +\%F -d "15 days"`'
              - '--end `date +\%F -d "15 days"`'
    strm-abc:
        prefix: '-strm-abc'
        monrun: True
        actions: 
          - name: 'notify-abc'
            schedule: 0 11 * * *
    strm-abc-devops:
        prefix: '-strm-abc-devops'
        monrun: True
        actions: 
          - name: 'notify-abc'
            schedule: 0 11 * * *

iface_ip_ignored_interfaces: 'lo|docker|dummy|vlan688|vlan788|vlan700'

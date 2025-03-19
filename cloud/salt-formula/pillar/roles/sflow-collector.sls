{%- set environment = grains['cluster_map']['environment'] -%}
{%- if environment == 'prod' -%}
{%- set ident = 'cloud-netinfra' -%}
{%- set log_type = 'fabric-sflow-log' -%}
{%- set anycast_loopback = '172.16.1.128' -%}
{% else %}
{%- set ident = 'cloud-netinfra@test' -%}
{%- set log_type = 'sflow' -%}
{%- set anycast_loopback = '172.16.1.129' -%}
{%- endif -%}
gobgp:
    sflow_anycast_loopback: {{ anycast_loopback }}
    sflow_community: 13238:35169
    tor_bgp_asn: 65401
    local_bgp_asn: 65402
    fc-hw1a-1.svc.hw1.cloud-lab.yandex.net:
        router_id: 172.16.4.47
# pre-prod hosts
    sflow-vla.svc.cloud-preprod.yandex.net:
        router_id: 172.16.4.32
    sflow-sas.svc.cloud-preprod.yandex.net:
        router_id: 172.16.4.33
    sflow-myt.svc.cloud-preprod.yandex.net:
        router_id: 172.16.4.34
# prod hosts
    sflow-vla.svc.cloud.yandex.net:
        router_id: 172.16.4.35
    sflow-sas.svc.cloud.yandex.net:
        router_id: 172.16.4.36
    sflow-myt.svc.cloud.yandex.net:
        router_id: 172.16.4.37

geo_bgp_communities:
    ru-lab1-a: 13238:35701
    ru-lab1-b: 13238:35701
    ru-lab1-c: 13238:35701
    ru-central1-a: 13238:35702
    ru-central1-b: 13238:35701
    ru-central1-c: 13238:35703


push_client:
  enabled: True
  # basic configs parameters

  instances:
    # override basic configs parameters for concrete instance of push-client
    sflow:
      enabled: True
      files:
        - name: /var/log/tflow/tskv-samples.log
          ident: {{ ident }}
          log_type: {{ log_type }}
          send_delay: 5


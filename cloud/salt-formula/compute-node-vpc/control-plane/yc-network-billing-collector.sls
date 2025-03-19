{% from 'compute-node-vpc/common/flow_log.jinja' import flow_log_directory, flow_log_prefix %}

/etc/yc/network-billing-collector/config.yaml:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/yc-network-billing-collector.conf
    - template: jinja
    - defaults:
        flow_log_directory: {{ flow_log_directory }}
        flow_log_prefix: {{ flow_log_prefix }}

yc_network_billing_collector_packages:
  yc_pkg.installed:
    - pkgs:
      - yc-network-billing-collector
    - hold_pinned_pkgs: True

yc-network-billing-collector:
  service.running:
    - enable: true
    - require:
      - file: /etc/yc/network-billing-collector/config.yaml
      - file: {{ flow_log_directory }}
      - yc_pkg: yc_network_billing_collector_packages
    - watch:
      - file: /etc/yc/network-billing-collector/config.yaml
      - file: {{ flow_log_directory }}
      - yc_pkg: yc_network_billing_collector_packages

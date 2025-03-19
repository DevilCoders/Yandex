{% from 'compute-node-vpc/common/flow_log.jinja' import flow_log_directory %}

{{ flow_log_directory }}:
  file.directory:
    - user: contrail
    - group: yc-network-billing-collector
    - mode: 0775
    - require:
      - user: {{ flow_log_directory }}
      - group: {{ flow_log_directory }}
  user.present:
    - name: contrail
  group.present:
    - name: yc-network-billing-collector

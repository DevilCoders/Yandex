{%- from slspath + "/map.jinja" import yarl_vars with context -%}

include:
  - .common

/etc/yarl/yarl.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yarl/yarl.yaml-master
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - context:
      vars: {{ yarl_vars }}

yarl-quotas:
  monrun.present:
    - command: 'res_file="/tmp/yarl-quotas-check.tmp"; if [[ -e $res_file ]]; then cat $res_file; else echo "1;file $res_file is missing"; fi'
    - execution_interval: 600
    - execution_timeout: 30
    - type: yarl

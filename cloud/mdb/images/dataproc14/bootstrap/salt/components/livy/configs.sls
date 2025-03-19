{% set config_path = '/etc/livy/conf' %}

{{ config_path }}/livy.conf:
  file.managed:
    - name: {{ config_path }}/livy.conf
    - source: salt://{{ slspath }}/conf/livy.conf
    - template: jinja
    - require:
      - pkg: livy_packages

/etc/default/livy-server:
  file.managed:
    - name: /etc/default/livy-server
    - source: salt://{{ slspath }}/conf/default
    - template: jinja
    - require:
      - pkg: livy_packages

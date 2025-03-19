include:
  - google-compute-engine-oslogin
  - breakglass

{% set config = pillar[sls]['config'] %}

openssh-server: pkg.installed

{{ config['trusted_user_ca_keys'] }}:
  file.managed:
    - user: root
    - group: root
    - mode: 0640
    - contents_pillar: {{ sls }}:ca_public_keys
    - require:
      - pkg: openssh-server

{{ sls }}:
  file.managed:
    - name: /etc/ssh/sshd_config
    - source: salt://{{ slspath }}/configs/sshd_config
    - user: root
    - group: root
    - mode: 0600
    - template: jinja
    - defaults: {{ config|yaml }}
    - require:
      - pkg: openssh-server
      - file: {{ config['trusted_user_ca_keys'] }}
      - sls: google-compute-engine-oslogin
      - sls: breakglass

  service.running:
    - enable: True
    - watch:
      - pkg: openssh-server
      - file: {{ sls }}

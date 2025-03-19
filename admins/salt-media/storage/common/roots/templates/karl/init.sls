{%- from slspath + "/map.jinja" import karl_vars with context -%}

include:
  - templates.karl-tls
  - .ubic

/etc/karl/karl.yaml:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/karl/karl.yaml
    - user: karl
    - group: karl
    - mode: 644
    - template: jinja
    - makedirs: true
    - context:
      vars: {{ karl_vars }}

/etc/logrotate.d/karl:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/karl
    - mode: 644
    - makedirs: true

/etc/karl/allCAs.pem:
  file.managed:
    - contents_pillar: tls_karl:karl_sslrootcert
    - mode: 644
    - user: karl
    - group: karl
    - makedirs: true

/etc/sudoers.d/karl-status:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/sudoers.d/karl-status
    - makedirs: True
    - mode: 440
    - user: root
    - group: root

karl-status:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/karl-status.sh
    - name: /usr/bin/karl-status.sh
    - mode: 755
    - makedirs: true

  monrun.present:
    - command: "/usr/bin/karl-status.sh 30"
    - execution_interval: 300
    - execution_timeout: 120
    - type: karl
    - makedirs: True

{% if karl_vars.is_control %}
karl-red-button:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/karl-red-button.sh
    - name: /usr/bin/karl-red-button.sh
    - mode: 755
    - makedirs: true

  monrun.present:
    - command: "/usr/bin/karl-red-button.sh 30"
    - execution_interval: 300
    - execution_timeout: 120
    - type: karl
    - makedirs: True
{% endif %}

check_mm_cache_path:
  file.directory:
    - user: root
    - name: "/var/cache/mastermind"
    - mode: 1777
    - group: root
    - makedirs: True

# MDS-18304
/usr/local/yasmagent/CONF/agent.karl.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/agent.karl.conf
    - user: root
    - group: root
    - mode: 644

restart-yasmagent-yasm-karl:
  pkg.installed:
    - pkgs:
      - yandex-yasmagent
  service.running:
    - name: yasmagent
    - enable: True
    - sig: yasmagent
    - watch:
      - file: /usr/local/yasmagent/CONF/agent.karl.conf

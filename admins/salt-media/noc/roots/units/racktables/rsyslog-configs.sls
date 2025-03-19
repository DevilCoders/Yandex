{% set isBSD = grains["kernel"] == "FreeBSD" %}
{% set prefix = "/usr/local" if isBSD else "" %}

include:
  - units.rsyslog.global

{% if isBSD %}
rsyslog-omlb:
  pkg.installed:
    - pkgs:
      - local/rsyslog-omlb
      - sysutils/rsyslog8

{{prefix}}/etc/rsyslog-omlb.yaml:
  file.managed:
    - user: root
    - mode: 740
    - contents: |
        log_level: error
        input:
          type: stdin
        output:
          endpoint: logbroker.yandex.net
          token: "{{pillar["sec_rt-yandex"]["robot_racktables_logbroker_token"]}}"
          topic: "/noc/nocdev/rt"
          source_id: "{{grains["fqdn"]}}"
          database: /Root
          use-tls: false
          codec: zstd
          confirm_messages: yes
        buffer_size: 100
{% endif %}

/var/spool/rsyslog:
  file.directory:
    - user: root

{{prefix}}/etc/rsyslog.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/rsyslog.conf
    - follow_symlinks: False
    - template: jinja
    - context:
        prefix: "{{prefix}}"

{{prefix}}/etc/rsyslog.d:
  file.managed:
    - makedirs: True
    - user: root
    - mode: 644
    - names:
      - {{prefix}}/etc/rsyslog.d/07-system-logs.conf:
        - source: salt://{{slspath}}/files/etc/rsyslog.d/07-system-logs.conf
      - {{prefix}}/etc/rsyslog.d/08-rt-logs.conf:
        - source: salt://{{slspath}}/files/etc/rsyslog.d/08-rt-logs.conf
      - {{prefix}}/etc/rsyslog.d/10-log-to-trapdoor.conf:
        - source: salt://{{slspath}}/files/etc/rsyslog.d/10-log-to-trapdoor.conf
      - {{prefix}}/etc/rsyslog.d/12-log-to-logbroker.conf:
        - source: salt://{{slspath}}/files/etc/rsyslog.d/12-log-to-logbroker.conf
        - template: jinja
      {% if isBSD %}
      - {{prefix}}/etc/rsyslog.d/09-fw-restart.conf:
        - source: salt://{{slspath}}/files/etc/rsyslog.d/09-fw-restart.conf
      {% endif %}




cleanup default rsyslog configs:
  file.absent:
    - names:
      - {{prefix}}/etc/rsyslog.d/20-ufw.conf
      - {{prefix}}/etc/rsyslog.d/21-cloudinit.conf
      - {{prefix}}/etc/rsyslog.d/50-default.conf
      # restructure
      - {{prefix}}/etc/rsyslog.d/11-invapi-access-log.conf
      - {{prefix}}/etc/rsyslog.d/12-journald-log.conf

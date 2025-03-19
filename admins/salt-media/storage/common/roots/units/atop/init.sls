{% set os_version = salt["grains.get"]("osrelease") %}

#{% if os_version == "14.04" %}
#kill_atop:
#  cmd.run:
#    - name: 'kill $(pidof atop)'
#    - unless:
#      - "if [[ $(pgrep -a atop | awk '{print $NF}') -eq 600 ]]; then echo True; fi"
#{% endif %}
atop_pkg:
  pkg.installed:
    - pkgs:
      {% if os_version == "18.04" %}
      - atop: 2.3.0-1
      {% elif os_version == "14.04" %}
      - atop: 1.27-10
      {% else %}
      - atop: latest
      {% endif %}

{% if os_version == "18.04" %}
/usr/share/atop/atop.daily:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/share/atop/atop.daily
    - user: root
    - group: root
    - mode: 755
    - require:
      - pkg: atop_pkg
{% elif os_version == "16.04" %}
/etc/default/atop:
  file.replace:
    - pattern: "^INTERVAL=.*"
    - repl: "INTERVAL=60"
    - append_if_not_found: True
    - require:
      - pkg: atop_pkg
/etc/cron.d/atop:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/cron.d/atop
    - user: root
    - group: root
    - mode: 644
    - require:
      - pkg: atop_pkg
{% else %}
/etc/logrotate.d/atop:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/atop
    - user: root
    - group: root
    - mode: 644
    - template: jinja
{% endif %}

# TODO: Fix atop for cloud*
{% if grains['fqdn'] not in ["cloud01sas.mds.yandex.net", "cloud01iva.mds.yandex.net"] %}

atop_service:
  service.running:
    - name: atop
    - enable: True
    - require:
      - pkg: atop_pkg
    - restart: True
{% if os_version == "18.04" %}
    - watch:
      - file: /usr/share/atop/atop.daily
{% elif os_version == "16.04" %}
    - watch:
      - file: /etc/default/atop
{% endif %}

{% endif %}

atop:
  file.managed:
    - name: /usr/local/bin/atop_check.sh
    - source: salt://{{ slspath }}/files/usr/local/bin/atop_check.sh
    - user: root
    - group: root
    - mode: 755
  monrun.present:
    - command: /usr/local/bin/atop_check.sh
    - execution_interval: 60
    - execution_timeout: 30

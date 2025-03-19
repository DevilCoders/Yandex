mdb-disklock-pkgs:
  pkg.installed:
    - pkgs:
        - mdb-disklock: '1.9614464'
    - require:
        - cmd: repositories-ready

{% if salt['pillar.get']('data:encryption:enabled', False) %}

/opt/yandex/mdb-disklock/mdb-disklock.yaml:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath + '/conf/mdb-disklock.yaml' }}
    - mode: '0640'
    - require:
        - pkg: mdb-disklock-pkgs

/etc/systemd/system/mdb-disklock.service:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath + '/conf/mdb-disklock.service' }}
    - require:
        - pkg: mdb-disklock-pkgs

mdb-disklock-service:
  service.enabled:
    - name: mdb-disklock
    - require:
        - pkg: mdb-disklock-pkgs
        - file: /opt/yandex/mdb-disklock/mdb-disklock.yaml
    - watch:
        - pkg: mdb-disklock-pkgs
        - file: /etc/systemd/system/mdb-disklock.service
    - require_in:
        - cmd: mount-data-directory

{% endif %}

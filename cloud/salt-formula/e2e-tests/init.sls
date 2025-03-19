yc-e2e-tests:
  yc_pkg.installed:
    - pkgs:
      - yc-e2e-tests

e2e-work-directories:
  file.directory:
    - user: root
    - group: root
    - dir_mode: 755
    - makedirs: True
    - names:
      - /var/log/e2e
      - /var/lib/yc/e2e-tests

/etc/yc/e2e-tests/pytest.ini:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/files/pytest.ini
    - require:
      - yc_pkg: yc-e2e-tests
      - file: e2e-work-directories

/etc/yc/e2e-tests/maintenance.ini:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/files/maintenance.ini
    - replace: False
    - require:
      - yc_pkg: yc-e2e-tests

/etc/yc/e2e-tests/secrets.ini:
  file.managed:
    - source: salt://{{ slspath }}/files/secrets.ini
    - replace: False
    - require:
      - yc_pkg: yc-e2e-tests

/etc/logrotate.d/yc-e2e-tests:
  file.managed:
    - source: salt://{{ slspath }}/files/yc-e2e-tests.logrotate
    - require:
      - yc_pkg: yc-e2e-tests

{%- set computes = salt['grains.get']('cluster_map:roles:compute') -%}
{%- if computes and grains['nodename'] == computes[0] %}
populate-e2e-database:
  cmd.run:
    - name: /usr/bin/yc-e2e-populate-database
    - require:
      - yc_pkg: yc-e2e-tests
      - file: /etc/yc/e2e-tests/pytest.ini
{%- endif %}

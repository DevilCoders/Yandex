include:
  - .vector_toml
{% if salt['pillar.get']('data:billing:ship_logs', True) %}
  - .billing
{% endif %}

vector-package:
  pkg.installed:
    - pkgs:
        - vector: 0.21.0-1

vector-validate-conf:
  cmd.run:
    - name: "vector validate --no-environment -C /etc/vector/"
    - cwd: /etc/vector/
    - runas: vector
    - require:
        - file: /etc/vector/vector.toml
        - pkg: vector-package
    - onchanges:
        - file: /etc/vector/vector.toml
        - file: /etc/vector/test_vector.toml

/var/log/salt/minion:
  file.touch:
    - onchanges:
        - file: /etc/vector/test_vector.toml

salt-log-perms:
  cmd.run:
    - name: "chmod 664 /var/log/salt/minion"
    - cwd: /etc/vector/
    - runas: root
    - require:
        - file: /var/log/salt/minion
    - onchanges:
        - file: /etc/vector/test_vector.toml

vector-test-conf:
  cmd.run:
    - name: "vector test -C /etc/vector"
    - cwd: /etc/vector/
    - runas: vector
    - require:
        - cmd: vector-validate-conf
        - file: /etc/vector/test_vector.toml
        - pkg: vector-package
        - cmd: salt-log-perms
    - onchanges:
        - file: /etc/vector/test_vector.toml

/etc/systemd/system/vector.service:
  file.managed:
    - template: jinja
    - require:
        - pkg: vector-package
    - require_in:
        - service: vector-service
    - source: salt://{{ slspath }}/conf/vector.service
    - mode: 644
    - onchanges_in:
        - module: systemd-reload

vector-service:
  service.running:
    - enable: true
    - name: vector
    - watch:
        - pkg: vector-package
        - file: /etc/vector/vector.toml
    - require:
        - cmd: vector-validate-conf
        - cmd: vector-test-conf

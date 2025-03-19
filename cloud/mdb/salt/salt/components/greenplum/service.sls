{% from "components/greenplum/map.jinja" import gpdbvars with context %}

greenplum-service:
  file.managed:
    - name: /lib/systemd/system/greenplum.service
    - source: salt://{{ slspath }}/conf/greenplum.service
    - user: root
    - group: root
    - mode: 0600
    - template: jinja
    - require:
      - sls: components.greenplum.install_greenplum
    - onchanges_in:
      - module: systemd-reload

/usr/local/yandex/gp_wait_cluster.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/gp_wait_cluster.py
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - mode: 755
        - require:
            - file: /usr/local/yandex
            - sls: components.greenplum.install_greenplum

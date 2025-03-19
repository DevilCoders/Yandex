{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

gp-cluster-failover-segment:
    cmd.run:
        - name: /usr/local/yandex/pre_restart.sh
        - runas: root
        - group: root
        - shell: /bin/bash

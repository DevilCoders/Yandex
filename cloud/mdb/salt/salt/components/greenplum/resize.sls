{% from "components/greenplum/map.jinja" import gpdbvars with context %}

/usr/local/yandex/gp_host_control.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/gp_host_control.py
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - mode: 755

compute-gp-pre-restart-script:
    file.accumulated:
        - name: compute-pre-restart
        - filename: /usr/local/yandex/pre_restart.sh
        - text: |-
            sudo -iu gpadmin bash -i << EOF
                /usr/local/yandex/gp_host_control.py --down
            EOF
        - require:
              - file: /usr/local/yandex/gp_host_control.py
        - require_in:
            - file: /usr/local/yandex/pre_restart.sh

compute-gp-post-restart-script:
    file.accumulated:
        - name: compute-post-restart
        - filename: /usr/local/yandex/post_restart.sh
        - text: |-
            sudo -iu gpadmin bash -i << EOF
                /usr/local/yandex/gp_host_control.py --up \
                    -r {{ not salt['pillar.get']('data:greenplum:config:segment_auto_rebalance_disable', False) }}
            EOF
        - require:
              - file: /usr/local/yandex/gp_host_control.py
        - require_in:
            - file: /usr/local/yandex/post_restart.sh

{% if salt.pillar.get('data:database_slice:enable', False) and salt.dbaas.is_dataplane() %}
database-slice-adjuster-deps:
    pkg.installed:
        - pkgs:
            - python3-portopy
        - prereq_in:
            - cmd: repositories-ready

/etc/cron.yandex/database_slice_adjuster.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/database_slice_adjuster.py
        - mode: '0755'
        - require:
            - pkg: database-slice-adjuster-deps

/etc/cron.d/database_slice_adjuster:
    file.managed:
        - source: salt://{{ slspath }}/conf/database_slice_adjuster.cron
        - require:
            - file: /etc/cron.yandex/database_slice_adjuster.py
            - file: /etc/database-slice-adjuster.conf
{% else %}
/etc/cron.yandex/database_slice_adjuster.py:
    file.absent

/etc/cron.d/database_slice_adjuster:
    file.absent
{% endif %}

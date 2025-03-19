/etc/mdb-metrics/conf.d/available/dbm_pgaas_resources.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbm_pgaas_resources.conf
        - mode: 644
        - makedirs: True
        - require:
            - pkg: mdb-metrics-pkg
        - require_in:
            - file: /etc/mdb-metrics/conf.d/enabled/dbm_pgaas_resources.conf
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/enabled/dbm_pgaas_resources.conf:
    file.symlink:
        - target: /etc/mdb-metrics/conf.d/available/dbm_pgaas_resources.conf
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/available/dbm_cores_stats.sql:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbm_cores_stats.sql
        - mode: 644
        - makedirs: True
        - require:
            - pkg: mdb-metrics-pkg
        - require_in:
            - file: /etc/mdb-metrics/conf.d/enabled/dbm_cores_stats.sql
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/enabled/dbm_cores_stats.sql:
    file.symlink:
        - target: /etc/mdb-metrics/conf.d/available/dbm_cores_stats.sql
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/available/dbm_ssd_flavors_stats.sql:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbm_ssd_flavors_stats.sql
        - mode: 644
        - makedirs: True
        - require:
            - pkg: mdb-metrics-pkg
        - require_in:
            - file: /etc/mdb-metrics/conf.d/enabled/dbm_ssd_flavors_stats.sql
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/enabled/dbm_ssd_flavors_stats.sql:
    file.symlink:
        - target: /etc/mdb-metrics/conf.d/available/dbm_ssd_flavors_stats.sql
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/available/dbm_hdd_flavors_stats.sql:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbm_hdd_flavors_stats.sql
        - mode: 644
        - makedirs: True
        - require:
            - pkg: mdb-metrics-pkg
        - require_in:
            - file: /etc/mdb-metrics/conf.d/enabled/dbm_hdd_flavors_stats.sql
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/enabled/dbm_hdd_flavors_stats.sql:
    file.symlink:
        - target: /etc/mdb-metrics/conf.d/available/dbm_hdd_flavors_stats.sql
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/available/dbm_dom0_free_ssd_space.sql:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbm_dom0_free_ssd_space.sql
        - mode: 644
        - makedirs: True
        - require:
            - pkg: mdb-metrics-pkg
        - require_in:
            - file: /etc/mdb-metrics/conf.d/enabled/dbm_dom0_free_ssd_space.sql
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/enabled/dbm_dom0_free_ssd_space.sql:
    file.symlink:
        - target: /etc/mdb-metrics/conf.d/available/dbm_dom0_free_ssd_space.sql
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/available/dbm_potential_ssd_allocations.sql:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbm_potential_ssd_allocations.sql
        - mode: 644
        - makedirs: True
        - require:
            - pkg: mdb-metrics-pkg
        - require_in:
            - file: /etc/mdb-metrics/conf.d/enabled/dbm_potential_ssd_allocations.sql
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/enabled/dbm_potential_ssd_allocations.sql:
    file.symlink:
        - target: /etc/mdb-metrics/conf.d/available/dbm_potential_ssd_allocations.sql
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/available/dbm_dom0_utilization.sql:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbm_dom0_utilization.sql
        - mode: 644
        - makedirs: True
        - require:
            - pkg: mdb-metrics-pkg
        - require_in:
            - file: /etc/mdb-metrics/conf.d/enabled/dbm_dom0_utilization.sql
        - watch_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/enabled/dbm_dom0_utilization.sql:
    file.symlink:
        - target: /etc/mdb-metrics/conf.d/available/dbm_dom0_utilization.sql
        - watch_in:
            - service: mdb-metrics-service

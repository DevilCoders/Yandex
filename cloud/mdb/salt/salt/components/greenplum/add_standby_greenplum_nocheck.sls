{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

{% if salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}

{% set status = salt['mdb_greenplum.get_master_replica_status']() %}

{%   if status == 'n' or salt.pillar.get('kill_replica') %}

{%     if status != 'n' %}
greenplum-replica-remove:
    cmd.run:
        - name: $GPHOME/bin/gpinitstandby -a -n
              -l {{ gpdbvars.gplog }}
              -r
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
/home/gpadmin/.ssh/known_hosts:
    file.absent:
        - require:
            - cmd: greenplum-replica-remove
{%     else %}
/home/gpadmin/.ssh/known_hosts:
    file.absent
{%     endif %}

gp-cluster-rm-data-folder:
    cmd.run:
        - name: ssh {{ salt.pillar.get('standby_master_fqdn') }} echo 1
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
        - require:
          - file: /home/gpadmin/.ssh/known_hosts

gp-cluster-initstandby:
    cmd.run:
        - name: $GPHOME/bin/gpinitstandby -a
              -l {{ gpdbvars.gplog }}
              -s {{ salt.pillar.get('standby_master_fqdn') }}
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
        - require:
          - cmd: gp-cluster-rm-data-folder
{%   elif status == 'd' %}
gp-cluster-initstandby-start:
    cmd.run:
        - name: $GPHOME/bin/gpinitstandby -a -n
              -l {{ gpdbvars.gplog }}
              -s {{ salt.pillar.get('standby_master_fqdn') }}
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
{%   elif status == 'u' %}
{%     if salt.pillar.get('check_if_exists') %}
gpdb_check_replica_master_is_up:
    mdb_greenplum.check_master_replica_is_alive_call

gp_cluster_initstandby_start:
    cmd.run:
        - name: $GPHOME/bin/gpinitstandby -a -n -l {{ gpdbvars.gplog }}
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
        - onfail:
          - mdb_greenplum: gpdb_check_replica_master_is_up

gp_cluster_replica_exists:
    cmd.run:
        - name: ssh {{ salt.pillar.get('standby_master_fqdn') }} echo 1
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
        - onfail:
          - cmd: gp_cluster_initstandby_start

gp_cluster_replica_folder_not_exists:
    cmd.run:
        - name: ssh {{ salt.pillar.get('standby_master_fqdn') }} test -f /var/lib/greenplum/data1/master/gpseg-1/pg_hba.conf
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
        - onfail:
          - cmd: gp_cluster_initstandby_start

gp_cluster_initstandby_remove:
    cmd.run:
        - name: $GPHOME/bin/gpinitstandby -a -r
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
        - onfail:
          - cmd: gp_cluster_replica_folder_not_exists
        - require:
          - cmd: gp_cluster_replica_exists

gp_cluster_initstandby:
    cmd.run:
        - name: $GPHOME/bin/gpinitstandby -a
              -l {{ gpdbvars.gplog }}
              -s {{ salt.pillar.get('standby_master_fqdn') }}
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
        - onfail:
          - cmd: gp_cluster_replica_folder_not_exists
        - require:
          - cmd: gp_cluster_replica_exists
          - cmd: gp_cluster_initstandby_remove

{%     else %}
greenplum-do_nothing-replica-master-is-online:
    test.fail_without_changes:
        - name: "The replica `{{ salt.pillar.get('standby_master_fqdn') }}` int `u` status!"
        - failhard: True

{%     endif %}
{%   else %}
greenplum-do_nothing-replica-status-unknown:
    test.fail_without_changes:
        - name: "The host `{{ salt.pillar.get('standby_master_fqdn') }}` is int `` status!"
        - failhard: True
{%   endif %}
{% else %}
greenplum-do_nothing-replica-master:
    test.fail_without_changes:
        - name: "The host `{{ salt.grains.get('id','') }}` is not master!"
        - failhard: True
{% endif %}

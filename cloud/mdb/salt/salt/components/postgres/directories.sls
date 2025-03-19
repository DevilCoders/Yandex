{% from "components/postgres/pg.jinja" import pg with context %}

/etc/postgresql/{{ pg.version.major }}/data:
    file.directory:
        - user: postgres
        - group: postgres
        - makedirs: True
        - require:
            - pkg: postgresql{{ pg.version.major }}-server
        - require_in:
            - service: postgresql-service

{{ pg.data }}:
{% if salt['pillar.get']('data:separate_array_for_data', True) %}
    mount.mounted:
        - device: {{ salt['pillar.get']('data:array_for_data', '/dev/md2')}}
        - fstype: ext4
        - mkmnt: True
        - opts:
            - defaults
            - noatime
            - nodiratime
        - onlyif:
            - grep "{{ salt['pillar.get']('data:array_for_data', '/dev/md/*2')}}" {{ pg.mdadm_config }}
{% endif %}
    file.directory:
        - user: postgres
        - group: postgres
        - mode: 0700
        - makedirs: True
        - require:
{% if salt['pillar.get']('data:separate_array_for_data', True) %}
            - mount: {{ pg.data }}
{% endif %}
            - pkg: postgresql{{ pg.version.major }}-server
        - require_in:
            - postgresql_cmd: pg-init
{% if pg.is_master %}
            - file: {{ pg.data }}/conf.d
{% endif %}
            - service: postgresql-service

{% if salt['pillar.get']('data:separate_array_for_data', True) %}
{{ pg.data }}/lost+found:
    file.absent:
        - require:
            - mount: {{ pg.data }}
        - require_in:
            - postgresql_cmd: pg-init
{% endif %}

{{ pg.prefix }}:
    file.directory:
        - user: postgres
        - group: postgres
        - mode: 0755
        - require:
            - pkg: postgresql{{ pg.version.major }}-server

{{ pg.prefix }}/{{ pg.version.major }}:
    file.directory:
        - user: postgres
        - group: postgres
        - mode: 0755
        - require:
            - file: {{ pg.prefix }}

{{ salt['pillar.get']('data:backup:archive:walsdir', pg.data + '/wals') }}:
    file.absent:
        - require:
            - file: {{ pg.data }}
            - postgresql_cmd: pg-init

{% if salt['pillar.get']('data:separate_array_for_xlogs', False) %}
{{ pg.wal_dir_path }}:
    {% if not salt['grains.get']('pg.role') %}
    mount.mounted:
        - device: {{ salt['pillar.get']('data:array_for_xlogs', '/dev/md3')}}
        - fstype: ext4
        - mkmnt: True
        - opts:
            - defaults
            - noatime
            - nodiratime
        - require_in:
            - file: {{ pg.wal_dir_path }}
            - file: {{ pg.wal_dir_path }}/lost+found
        - unless:
            - "mount -l | grep '{{ pg.wal_dir_path }}'"
    {% endif %}
    file.directory:
        - user: postgres
        - group: postgres
        - mode: 0700
        - require:
            - mount: {{ pg.wal_dir_path }}
        - require_in:
            - service: postgresql-service

{{ pg.wal_dir_path }}/lost+found:
    file.absent
{% endif %}

{% if salt['pillar.get']('data:separate_array_for_sata', False) %}
{{ pg.prefix }}/slow:
    mount.mounted:
        - device: {{ salt['pillar.get']('data:array_for_sata', '/dev/md4')}}
        - fstype: ext4
        - mkmnt: True
        - opts:
            - defaults
            - noatime
            - nodiratime
        - require_in:
            - file: {{ pg.prefix }}/slow
            - file: {{ pg.prefix }}/slow/lost+found
    file.directory:
        - user: postgres
        - group: postgres
        - mode: 0700
        - require_in:
            - service: postgresql-service

{{ pg.prefix }}/slow/lost+found:
    file.absent
{% endif %}

{{ pg.prefix }}/.ssh:
    file.directory:
        - user: postgres
        - group: postgres
        - mode: 700

{{ pg.prefix }}/.ssh/ms:
    file.directory:
        - user: postgres
        - group: postgres
        - mode: 700
        - require:
            - file: {{ pg.prefix }}/.ssh

/var/log/postgresql:
    file.directory:
        - user: root
        - group: postgres
        - mode: 775
        - require_in:
            - directory: {{ pg.data }}

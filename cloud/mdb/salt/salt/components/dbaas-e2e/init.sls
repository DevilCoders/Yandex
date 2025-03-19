{% set ca_path = '/home/monitor/root.crt' %}
{% set timeout = '1800' %}
dbaas-e2e-pkgs:
    pkg.installed:
        - pkgs:
            - dbaas-e2e: '1.9787013'

{% set test_cases = salt['pillar.get']('data:dbaas-e2e:test_cases', {}) %}
{% set all_test_cases = ['compute-qa', 'porto-qa', 'sdk-qa'] %}

/etc/logrotate.d/dbaas-e2e:
    file.managed:
        - source: salt://{{ slspath }}/logrotate.conf
        - mode: 644
        - makedirs: True

{% for test_case in test_cases %}
{% set opts = test_cases[test_case] %}
/etc/dbaas-e2e-{{ test_case }}.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/dbaas-e2e.conf
        - mode: '0640'
        - user: root
        - group: monitor
        - defaults:
            ca_path: {{ ca_path }}
            conn_ca_path: {{ ca_path }}
            timeout: {{ timeout }}
            oauth_token: {{ salt['pillar.get']('data:dbaas-e2e:oauth_token') }}
            api_url: {{ salt['pillar.get']('data:dbaas-e2e:api_url') }}
            grpc_api_url: {{ salt['pillar.get']('data:dbaas-e2e:grpc_api_url') }}
            ssh_public_key: {{ salt['pillar.get']('data:dbaas-e2e:ssh_public_key') }}
            s3_bucket: {{ salt['pillar.get']('data:dbaas-e2e:s3_bucket') }}
            hadoop_image_version: {{ salt['pillar.get']('data:dbaas-e2e:hadoop_image_version') }}
            hadoop_flavor: {{ salt['pillar.get']('data:dbaas-e2e:hadoop_flavor') }}
            greenplum_master_flavor: {{ salt['pillar.get']('data:dbaas-e2e:greenplum_master_flavor') }}
            greenplum_segment_flavor: {{ salt['pillar.get']('data:dbaas-e2e:greenplum_segment_flavor') }}
        - context: {{ opts | yaml }}
        - require:
            - pkg: dbaas-e2e-pkgs

/etc/monrun/conf.d/dbaas-e2e-{{ test_case }}.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/monrun.conf
        - defaults:
            name: {{ test_case }}
            execution_timeout: {{ salt['pillar.get']('data:dbaas-e2e:execution_timeout', 3600) }}
            execution_interval: {{ salt['pillar.get']('data:dbaas-e2e:execution_interval', 3600) }}
        - context: {{ opts | yaml }}
        - require:
            - file: /etc/dbaas-e2e-{{ test_case }}.conf
        - watch_in:
            - cmd: monrun-jobs-update
{% endfor %}

{% for test_case in all_test_cases %}
{% if test_case not in test_cases %}
/etc/dbaas-e2e-{{ test_case }}.conf:
    file.absent

/etc/monrun/conf.d/dbaas-e2e-{{ test_case }}.conf:
    file.absent:
        - watch_in:
            - cmd: monrun-jobs-update
{% endif %}
{% endfor %}

{{ ca_path }}:
    file.managed:
        - contents_pillar: cert.ca
        - template: jinja
        - user: monitor
        - group: monitor
        - mode: 600

/home/monitor/.ssh/dataproc_ed25519:
    file.managed:
        - source: salt://{{ slspath }}/conf/dataproc_ed25519
        - template: jinja
        - mode: 600
        - user: monitor
        - group: monitor


/root/.ssh/dataproc_ed25519:
    file.symlink:
        - target: /home/monitor/.ssh/dataproc_ed25519
        - mode: 600
        - force: True

odbc-packages:
    pkg.installed:
        - pkgs:
            - unixodbc: 2.3.7+yandex0
            - odbcinst1debian2: 2.3.7+yandex0
            - libodbc1: 2.3.7+yandex0
            - odbcinst: 2.3.7+yandex0
            - msodbcsql17: 17.4.2.1-1+yandex0
            - python-pyodbc

/opt/yandex/odbc_check.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/odbc_check.py
        - mode: 755
        - user: monitor
        - group: monitor

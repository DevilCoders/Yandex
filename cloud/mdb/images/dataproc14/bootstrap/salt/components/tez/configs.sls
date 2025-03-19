{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}
{% set services = salt['pillar.get']('data:services') %}

{% set hadoop_config_path = salt['pillar.get']('data:config:hadoop_config_path', '/etc/hadoop/conf') %}
{% set hive_config_path = salt['pillar.get']('data:config:hive_config_path', '/etc/hive/conf') %}
{% set tez_config_path = salt['pillar.get']('data:config:tez_config_path', '/etc/tez/conf') %}
{% set ui_proxy_url = salt['ydputils.get_ui_proxy_url']() %}

{% if 'yarn' in services -%}
tez-patch-yarn-site.xml:
    hadoop-property.present:
        - config_path: {{ hadoop_config_path }}/yarn-site.xml
        - properties:
            'yarn.timeline-service.ui-names': 'tez'
            'yarn.timeline-service.ui-on-disk-path.tez': '/usr/lib/tez/tez-ui-0.9.2.war'
            'yarn.timeline-service.ui-web-path.tez': '/tez-ui'
        - require_in:
            - file: {{ hadoop_config_path }}/yarn-site.xml

tez-patch-yarn-env.sh:
    file.append:
        - name: {{ hadoop_config_path }}/yarn-env.sh
        - text:
            - HADOOP_CLASSPATH="${HADOOP_CLASSPATH}:/usr/lib/tez/*:/usr/lib/tez/lib/*:/etc/tez/conf:"
{%- endif %}

{% import 'components/tez/tez-site.sls' as c with context %}
{{ tez_config_path }}/tez-site.xml:
    hadoop-property.present:
        - config_path: {{ tez_config_path }}/tez-site.xml
        - properties:
            {{ c.config_site['tez'] | json }}

# MDB-10366: Fix bug with wrong tez links
# Some of configs inside war file and we must rewrite them
{% if salt['ydputils.check_roles'](['masternode']) -%}
{{ tez_config_path }}/configs.env:
    file.managed:
        - name: {{ tez_config_path }}/configs.env
        - source: salt://{{ slspath }}/conf/configs.env
        - template: jinja
        - require:
            - pkg: tez_packages

{% if ui_proxy_url %}
{{ tez_config_path }}/configs.js:
    file.managed:
        - name: {{ tez_config_path }}/configs.js
        - source: salt://{{ slspath }}/conf/configs.js
        - template: jinja
        - require:
            - pkg: tez_packages
        - defaults:
            cluster_ui_hostname: {{ ui_proxy_url }}
{% endif %}

/usr/lib/tez/tez-ui-0.9.2.war:
    zipfile.objects_present:
        - archive_path: /usr/lib/tez/tez-ui-0.9.2.war
        - content:
            'config/configs.env': {{ tez_config_path }}/configs.env
{% if ui_proxy_url %}
            'config/configs.js': {{ tez_config_path }}/configs.js
{% endif %}
        - require:
            - pkg: tez_packages
            - file: {{ tez_config_path }}/configs.env
{% if ui_proxy_url %}
            - file: {{ tez_config_path }}/configs.js
{% endif %}
        - require_in:
            - service: service-hadoop-yarn-resourcemanager
            - service: service-hadoop-yarn-timelineserver
{% endif %}

{% if salt['ydputils.check_roles'](['masternode']) -%}
{% if 'hive' in services -%}
tez-patch-hive-site.xml:
    hadoop-property.present:
        - config_path: {{ hive_config_path }}/hive-site.xml
        - properties:
            'hive.execution.engine': 'tez'
{% if salt['ydputils.check_roles'](['masternode']) %}
        - require_in:
            - service: service-hive-server2
{% endif %}

{%- endif %}
{%- endif %}

{%- set all_nodes = salt['pillar.get']('data:properties:monitoring:all_nodes', False) -%}

{% if all_nodes or salt['ydputils.is_masternode']() %}
{% if salt['ydputils.is_presetup']() %}
telegraf-repository:
    pkgrepo.managed:
        - name: deb https://repos.influxdata.com/ubuntu/ focal stable
        - file: /etc/apt/sources.list.d/influxdata.list
        - key_url: https://repos.influxdata.com/influxdb.key
        - gpgcheck: 1
        - refresh: True
        - require_in:
            - pkg: telegraf-packages

{% endif %}

telegraf-packages:
    pkg.installed:
        - refresh: False
        - pkgs:
            - telegraf

{% if salt['ydputils.is_presetup']() %}
service-telegraf-disabled:
    service.disabled:
        - name: telegraf
        - require:
            - pkg: telegraf-packages
{% endif %}

{% if not salt['ydputils.is_presetup']() %}
/etc/telegraf/telegraf.conf:
    file.managed:
        - template: jinja
        - makedirs: true
        - show_changes: false
        - source: salt://{{ slspath }}/conf/telegraf.conf
        - require:
            - pkg: telegraf-packages

/etc/telegraf/telegraf.conf.sample:
    file.absent:
        - show_changes: false

service-telegraf:
    service:
        - running
        - enable: true
        - name: telegraf
        - require:
            - file: /etc/telegraf/telegraf.conf
            - file: /etc/telegraf/telegraf.conf.sample
            - pkg: telegraf-packages
        - watch:
            - file: /etc/telegraf/telegraf.conf
            - pkg: telegraf-packages
{% endif %}

{% endif %}

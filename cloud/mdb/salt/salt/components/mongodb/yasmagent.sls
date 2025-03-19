{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}
{% set prefix = '/usr/local' %}
{% set use_yasmagent = salt.pillar.get('data:use_yasmagent', True) %}


{% for srv in salt.mdb_mongodb_helpers.all_services() %}
yasmagent-instance-getter-{{ srv }}:
    file.managed:
        - name: {{ prefix }}/yasmagent/mdb_{{srv}}_getter.py
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.getter.py
        - makedirs: True
        - mode: 755
        - context:
          srv: {{ srv | tojson}}
        - defaults:
            itypes: {{ salt.pillar.get('data:yasmagent:instances', ['mdbmongodb'])|join(',') }}
            ctype: {{ salt.pillar.get('data:yasmagent:ctype', salt.grains.get('ya:group', '')) }}
        - require:
{% if use_yasmagent %}
            - pkg: yasmagent-packages
{% else %}
            - pkg: yandex-yasmagent
{% endif %}
{% endfor %}

yasmagent-instance-getter:
    file.managed:
        - name: {{ prefix }}/yasmagent/mdb_mongodb_getter.py
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.getter.py
        - makedirs: True
        - mode: 755
        - context:
          srv: {{ mongodb.services_deployed[0] | tojson }}
        - defaults:
            itypes: {{ salt.pillar.get('data:yasmagent:instances', ['mdbmongodb'])|join(',') }}
            ctype: {{ salt.pillar.get('data:yasmagent:ctype', salt.grains.get('ya:group', '')) }}
        - require:
{% if use_yasmagent %}
            - pkg: yasmagent-packages
{% else %}
            - pkg: yandex-yasmagent
{% endif %}

{% if use_yasmagent %}
yasmagent-packages:
    pkg.installed:
        - pkgs:
            - yandex-yasmagent: 2.279-20190206
        - require:
            - pkgrepo: mdb-bionic-stable-all

yasmagent-init:
    file.managed:
        - name: /etc/init.d/yasmagent
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.init
        - mode: 755
        - require:
            - pkg: yasmagent-packages

yasmagent-default-config:
    file.managed:
        - name: /etc/default/yasmagent
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.default
        - mode: 644

yasmagent-restart:
    service:
        - running
        - enable: true
        - name: yasmagent
        - watch:
            - file: yasmagent-instance-getter
            - file: yasmagent-default-config
            - file: yasmagent-init

{% else %}

yandex-yasmagent:
    pkg.purged
{% endif %}

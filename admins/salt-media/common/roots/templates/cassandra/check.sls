{% from  slspath + "/map.jinja" import cassandra,keyspaces_for_ntstatus,mx4j with context %}
{% set monrun = [] %}

/etc/monitoring:
  file.recurse:
    - template: jinja
    - source: salt://{{ slspath }}/etc/monitoring
    - makedirs: True
    - include_empty: True
    - require_in:
      - cmd: regenerate-monrun-tasks
      - pkg: cassandra-monrun-package
    - context:
      keyspaces: {{ keyspaces_for_ntstatus }}
      monrun: {{ cassandra['monrun']|yaml }}

{%- for ks, data in mx4j.iteritems() %}
  {%- for sp,sdata in data.iteritems() %}
    {%- for chk,cdata in sdata.iteritems() %}
    {%- if cdata is mapping %}
      {%- set int = cdata.pop("interval", 600) %}
    {%- else %}
      {%- if cdata|length > 2 %}{%- set int = cdata.pop(2) %}
      {%- else %}               {%- set int = 600 %}         {% endif %}
    {%- endif %}
    {%- set desc = cassandra.mx4j.checks[chk] %}
    {%- if desc.type is not none %}
      {%- set name = "cassandra-mx4j-{0}-{1}-{2}-{3}".format(ks,sp,chk,desc.type) %}
    {%- else %}
      {%- set name = "cassandra-mx4j-{0}-{1}-{2}".format(ks,sp,chk) %}
    {%- endif %}
    {%- set conf = "/etc/monitoring/{0}.conf".format(name) %}

{{ conf }}:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/mx4j.tmpl
    - require_in:
      - pkg: cassandra-monrun-package
      - cmd: regenerate-monrun-tasks
    - context:
        keyspace: {{ ks }}
        scope: {{ sp }}
        check: {{ desc|json }}
        param: {{ cdata|json }}

{%- do monrun.append(cassandra.mx4j.tmpl.format(name=name,interval=int,conf=conf)) %}
{%- endfor %}{% endfor %}{% endfor %}

/etc/monrun/conf.d/cassandra-monitoring.conf:
  file.managed:
    - makedirs: True
    - require_in:
      - cmd: regenerate-monrun-tasks
      contents: |
        {{ monrun|join("\n")|indent(8) }}

/etc/monrun/conf.d/cassandra.conf:
  file.managed:
    - makedirs: True
    - require_in:
      - cmd: regenerate-monrun-tasks
      contents: |
        [cassandra]
        execution_interval=60
        execution_timeout=30
        command=/etc/init.d/cassandra status > /dev/null; if [ $? -gt 0 ]; then echo "2;Cassandra not running"; else echo "0;ok"; fi
        type=cassandra

regenerate-monrun-tasks:
  cmd.wait:
    - name: /usr/sbin/regenerate-monrun-tasks
    - require:
      - file: /etc/monrun/conf.d/cassandra-monitoring.conf

cassandra-monrun-package:
  pkg.installed:
  - pkgs:
      - config-monrun-cassandra


{%- if cassandra.get("mx4j_tools", True) %}
/usr/share/cassandra/lib/mx4j-tools.jar:
  file.managed:
    - source: salt://{{ slspath }}/usr/share/cassandra/lib/mx4j-tools.jar
{% endif %}

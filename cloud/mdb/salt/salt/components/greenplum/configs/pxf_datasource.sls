{% from "components/greenplum/map.jinja" import gpdbvars,sysvars,pxfvars with context %}

{% if salt.pillar.get('data:pxf:install', True) %}
{{ pxfvars.pxfconf }}/servers:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0750
    - clean: True
    - require:
      - file: /lib/systemd/system/{{ pxfvars.service_name }}.service
      - file: {{ pxfvars.pxfconf }}/servers/default
{%   if salt.pillar.get('data:pxf:datasources:jdbc',{}) %}
{%     for datasource,datasource_params in salt.pillar.get('data:pxf:datasources:jdbc').items() %}
      - file: {{ pxfvars.pxfconf }}/servers/{{ datasource }}
{%     endfor %}
{%   endif %}
{%   if salt.pillar.get('data:pxf:datasources:s3',{}) %}
{%     for datasource,datasource_params in salt.pillar.get('data:pxf:datasources:s3').items() %}
      - file: {{ pxfvars.pxfconf }}/servers/{{ datasource }}
{%     endfor %}
{%   endif %}

{%   if salt.pillar.get('data:pxf:datasources:jdbc',{}) %}
{%     for datasource,datasource_params in salt.pillar.get('data:pxf:datasources:jdbc').items() %}
{{ pxfvars.pxfconf }}/servers/{{ datasource }}:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0750

{{ pxfvars.pxfconf }}/servers/{{ datasource }}/jdbc-site.xml:
  file.managed:
    - source: salt://components/greenplum/conf/pxf/jdbc-site.xml
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0640
    - template: jinja
    - context:
      datasource: {{ datasource | tojson }}
{%     endfor %}
{%   endif %}

{%   if salt.pillar.get('data:pxf:datasources:s3',{}) %}
{%     for datasource,datasource_params in salt.pillar.get('data:pxf:datasources:s3').items() %}
{{ pxfvars.pxfconf }}/servers/{{ datasource }}:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0750

{{ pxfvars.pxfconf }}/servers/{{ datasource }}/s3-site.xml:
  file.managed:
    - source: salt://components/greenplum/conf/pxf/s3-site.xml
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0640
    - template: jinja
    - context:
      datasource: {{ datasource | tojson }}
{%     endfor %}
{%   endif %}

{{ pxfvars.pxfconf }}/servers/default:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0750

{{ pxfvars.pxfconf }}/servers/default/s3-site.xml:
  file.managed:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0640
    - source: salt://components/greenplum/conf/pxf/default-s3-site.xml
    - template: jinja
    - require:
      - {{ pxfvars.pxfconf }}/servers/default
{% endif %}

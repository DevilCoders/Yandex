{% from slspath + "/map.jinja" import certificates with context %}

{% set environ = "" %}
{%- if certificates.saltenv %}
  {%- set environ = "?saltenv={0}".format(certificates.saltenv) %}
{%- endif %}


{%- for crt in certificates.get('files', []) %}
{{ certificates.path }}/{{ crt }}:
  yafile.managed:
    - source: salt://{{ certificates.source }}/{{ crt }}{{ environ }}
    - makedirs: True
    - user: {{ certificates.cert_owner }}
    - group: {{ certificates.cert_group }}
    - mode: "{{ certificates.get('modes', {}).get(crt, '0400') }}"
    - require:
      - user: certificates_owner
      - group: certificates_group
{%- endfor %}

{%- for crt_name, crt_content in certificates.get('contents', {}).items() %}
{{ certificates.path }}/{{ crt_name }}:
  file.managed:
    - contents: {{ crt_content | json }}
    - makedirs: True
    - user: {{ certificates.cert_owner }}
    - group: {{ certificates.cert_group }}
    - mode: "0400"
    - require:
      - user: certificates_owner
      - group: certificates_group
{%- endfor %}

{% set from_pillar = certificates.get('from_pillar') %}
{% if from_pillar and from_pillar.get('name') and from_pillar.get('files') %}
{% set cert_from_pillar = salt['pillar.get'](from_pillar.name, default={}) %}
{% for crt_name in from_pillar.files %}
{{ certificates.path }}/{{ crt_name }}:
{% if crt_name not in cert_from_pillar.keys() %}
  cmd.run:
    - name: echo "There is no {{ crt_name }} key in {{ from_pillar.name }} pillar";exit 1
{% else %}
  file.managed:
    - contents: {{ cert_from_pillar[crt_name] | json }}
    - makedirs: True
    - user: {{ certificates.cert_owner }}
    - group: {{ certificates.cert_group }}
    - mode: "0400"
    - require:
      - user: certificates_owner
      - group: certificates_group
{% endif %}
{% endfor %}
{% endif %}

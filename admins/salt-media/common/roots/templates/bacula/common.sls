{% set path = slspath if slspath.endswith('/bacula') else slspath.split('/')[0] %}
{% from path + "/map.jinja" import bacula with context %}

{% set env = grains.get('yandex-environment', 'FAKE').replace('production', 'stable') %}

{% set c = bacula.get(component) %}
{% set secret = c.get("secret_file", grains["fqdn"]) %}
{% set secret_file = bacula.secrets.dst + '/' + secret %}

{{component}}:
  pkg.installed:
    - pkgs: {{ salt.conductor.package(c.pkg) or c.pkg }}
    - install_recommends: False
{%- if c.get("service") %}
  service.running:
    - name: {{ c.service }}
    - watch:
      {%- for cfg in c.conf %}
      - file: {{ cfg }}
      {%- endfor %}
      - file: {{ secret_file }}
{%- endif %}

{%- for cfg in c.conf %}
{{ cfg }}:
  file.managed:
    - makedirs: True
    - source:
      - salt://{{ path }}{{ cfg.replace('.conf', '.j2') }}?saltenv={{ env }}
      - salt://{{ path }}{{ cfg.replace('.conf', '.j2') }}
    - template: jinja
    - context:
      bacula: {{ bacula }}
      secret_file: {{ secret_file }}
{%- endfor %}


{{ component}}_{{ secret_file }}:
  file.managed:
    - name: {{ secret_file }}
    - makedirs: True
    - source:
      - {{ bacula.secrets.src }}/{{ secret }}?saltenv={{ env }}
      - {{ bacula.secrets.src }}/{{ secret }}
      - {{ bacula.secrets.src }}/{{ component }}?saltenv={{ env }}
      - {{ bacula.secrets.src }}/{{ component }}
    - require_in:
      {%- for cfg in c.conf %}
      - file: {{ cfg }}
      {%- endfor %}

{% for name, dir in bacula.get("dirs", {}).items() %}
{%- if name in c.get("makedirs", []) %}
{{ component}}_{{ name }}:
  file.directory:
    - name: {{ dir }}
    - makedirs: True
    - user: bacula
    - group: bacula
    - dir_mode: "0750"
    {%- if c.get("service") %}
    - require_in:
      - service: {{ c.service }}
    {%- endif %}
{%- endif %}
{% endfor %}

### only for director
{% if component == 'dir' %}
{% for client in bacula.clients %}
director_secret_for_clients_{{ client }}:
  file.managed:
    - name: {{ bacula.secrets.dst + "/" + client }}
    - makedirs: True
    - source:
      - {{ bacula.secrets.src }}/{{ client }}?saltenv={{ env }}
      - {{ bacula.secrets.src }}/{{ client }}
      - {{ bacula.secrets.src }}/fd?saltenv={{ env }}
      - {{ bacula.secrets.src }}/fd
    - require_in:
      - service: {{c.service}}
{% endfor %}
{% endif %}

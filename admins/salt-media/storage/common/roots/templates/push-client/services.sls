{% from slspath + "/map.jinja" import push_client with context %}

{%- if push_client.get("ssl") %}
include:
  - .allCAs
{%- endif %}

push_client_packages:
  pkg.installed:
    - pkgs:
    {%- for pkg in push_client.packages %}
      - {{ pkg }}
    {%- endfor %}
    {%- if push_client.get("ssl") %}
    - require:
      - file: allCAs_pem
    {%- endif %}

  service.running:
    - name: {{ push_client.service }}
    - enable: True
    - sig: push-client -f -w

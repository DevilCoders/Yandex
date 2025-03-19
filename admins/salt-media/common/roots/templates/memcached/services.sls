{% from slspath + "/map.jinja" import memcached with context %}
memcached_packages:
  pkg.installed:
    - pkgs:
    {%- for pkg in memcached.packages %}
      - {{ pkg }}
    {%- endfor %}
    - refresh: True
    - allow_updates: True

  {% if memcached.restart %}
  service.running:
    - name: {{ memcached.service }}
    - enable: True
    - watch:
      - file: /etc/memcached.conf
    - require:
      - file: /etc/memcached.conf
  {% else %}
  cmd.wait:
    - name: echo "memcached config changed but restart disabled in pillar! Please restart it carefully by hands." && false
    - watch:
      - file: /etc/memcached.conf
    - require:
      - file: /etc/memcached.conf
  {% endif %}

{% if memcached.mcrouter is defined %}
{% if salt["grains.get"]("osrelease") >= 16.04 %}
{% set osname = salt["grains.get"]("oscodename") %}
mcrouter_repo_key:
  cmd.run:
    - name: wget -O - https://facebook.github.io/mcrouter/debrepo/{{ osname }}/PUBLIC.KEY | sudo apt-key add
    - unless: dpkg -l mcrouter

mcrouter_repo:
  cmd.run:
    - name: echo "deb https://facebook.github.io/mcrouter/debrepo/{{ osname }} {{ osname }} contrib" > /etc/apt/sources.list.d/mcrouter.list
    - unless: dpkg -l mcrouter
{% endif %}

mcrouter_service:
  service.running:
    - name: mcrouter
    - enable: True
    - watch:
      - file: /etc/mcrouter/mcrouter.conf
{% endif %}

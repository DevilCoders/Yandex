{# CADMIN-4414 #}
{% from slspath + "/map.jinja" import memcached with context %}
{% if salt["grains.get"]("osrelease") in ["12.04", "14.04"] and memcached.legacy_os_repo %}
ubuntu-toolchain-repo:
  pkgrepo.managed:
    - ppa: ubuntu-toolchain-r/test

libstdc++6:
  pkg.installed:
    - refresh: True
    - require:
      - pkgrepo: ubuntu-toolchain-repo
{% endif %}

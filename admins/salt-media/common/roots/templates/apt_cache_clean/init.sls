{% from slspath + "/map.jinja" import apt_cache_clean with context %}
/etc/apt/apt.conf.d/99apt_cache_clean:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - contents: DPkg::Post-Invoke {"/usr/bin/find {{ apt_cache_clean.path }} -name '{{ apt_cache_clean.name }}' -mtime {{ apt_cache_clean.mtime }} -delete";};

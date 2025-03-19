/etc/nginx/sites-available/44-unistat.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-available/44-unistat.conf
    - user: root
    - group: root
    - mode: 644
    - makedirs: true

/etc/nginx/sites-enabled/44-unistat.conf:
  file.symlink:
    - target: /etc/nginx/sites-available/44-unistat.conf
    - makedirs: true
    - require:
      - file: /etc/nginx/sites-available/44-unistat.conf


{% for filename in ['worker', 'metrics', 's3-metrics', 'metrics_storage'] %}
/etc/nginx/include/{{ filename }}.lua:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/include/{{ filename }}.lua
    - user: root
    - group: root
    - mode: 644
    - makedirs: true
    - template: jinja
{% endfor %}

lua-cjson:
  pkg.installed:
    - pkgs:
      - lua-cjson

lua-lfs:
  pkg.installed:
    - pkgs:
      - lua-filesystem

{% if pillar.get('libmastermind_cache_path') %}
/usr/bin/mmc-list-namespaces:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/mmc-list-namespaces
    - user: root
    - group: root
    - mode: 755

/usr/bin/mm_namespaces.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/mm_namespaces.sh
    - user: root
    - group: root
    - mode: 755
    - template: jinja

/etc/cron.d/mm_namespaces:
  file.managed:
    - contents: >
        */10 * * * * root /usr/bin/mm_namespaces.sh
    - user: root
    - group: root
    - mode: 644
{% endif %}

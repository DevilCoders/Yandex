yandex-conf-repo-strm-stable:
  pkg.installed:
    - pkgs:
      - yandex-conf-repo-strm-stable

strm-playlist-generator:
  pkg.installed:
    - pkgs:
      - strm-playlist-generator

{% set playlist_count = 14 %}
{% set playlist_start_port = 8884 %}
{% set playlist_ports = [] %}

{% for p in range(playlist_count) %}
{% set playlist_port = p + playlist_start_port %}
{% do playlist_ports.append(playlist_port) %}

{% if p == 0 %}
/etc/ubic/service/playlist.json:
{% else %}
/etc/ubic/service/playlist{{ p + 1 }}.json:
{% endif %}
  yafile.managed:
    - source: "salt://files/strm-playlist-generator/playlist.json"
    - template: "jinja"
    - user: root
    - file_mode: 644
    - context:
      playlist_port: {{ playlist_port }}
{% endfor %}

{% for p in range(playlist_count, 100) %}
/etc/ubic/service/playlist{{ p + 1 }}.json:
  file.absent
{% endfor %}

# kostil
# /etc/nginx/sites-available/55-strm-playlist.conf:
/etc/nginx/sites-enabled/55-strm-playlist.conf:
  file.managed:
    - source: salt://files/strm-playlist-generator/55-strm-playlist.conf
    - user: root
    - group: root
    - mode: 644
    - template: "jinja"
    - context:
      playlist_ports: {{ playlist_ports }}

# /etc/nginx/sites-enabled/55-strm-playlist.conf:
#   file.symlink:
#     - target: /etc/nginx/sites-available/55-strm-playlist.conf
#     - require:
#       - file: /etc/nginx/sites-available/55-strm-playlist.conf

/usr/local/yasmagent/CONF/agent.strmstream.conf:
  file.managed:
    - source: salt://files/strm-playlist-generator/agent.strmstream.conf
    - user: root
    - group: root
    - mode: 644
    - template: "jinja"
    - context:
      playlist_ports: {{ playlist_ports }}

restart-yasmagent-strm:
  pkg.installed:
    - pkgs:
      - yandex-yasmagent
  service.running:
    - name: yasmagent
    - enable: True
    - sig: yasmagent
    - watch:
      - file: /usr/local/yasmagent/CONF/agent.strmstream.conf

/etc/logrotate.d/strm:
  file.managed:
    - source: salt://files/strm-playlist-generator/strm.logrotate
    - user: root
    - group: root
    - mode: 644

/etc/nginx/strm:
  file.recurse:
    - source: salt://files/strm-playlist-generator/strm
    - user: root
    - group: root
    - dir_mode: 755
    - file_mode: 644

/etc/playlist-generator/playlist.conf:
  file.managed:
    - source: salt://files/strm-playlist-generator/playlist.conf
    - user: root
    - group: root
    - mode: 644

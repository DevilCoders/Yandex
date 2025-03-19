{%- from slspath + "/map.jinja" import drooz_nginx_local_conf with context -%}


create_tmpfs_mm:
  file.directory:
    - user: root
    - name: "/var/cache/mastermind"
    - mode: 1777
    - group: root
    - makedirs: True
  mount.mounted:
    - name: "/var/cache/mastermind"
    - device: tmpfs
    - fstype: tmpfs
    - opts: defaults,nodev,nosuid,size=3G
    - persist: True

/etc/nginx/sites-enabled/20-drooz.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/20-drooz.conf
    - user: root
    - group: root
    - mode: 644
    - template: jinja
    - context:
      vars: {{ drooz_nginx_local_conf }}

  service.running:
    - name: nginx
    - reload: True
    - watch:
      - file: /etc/nginx/sites-enabled/20-drooz.conf

cgroup-bin:
  pkg.installed

mongodb_init_with_cgroups:
  file.managed:
    - name: /etc/init/mongodb.conf
    - user: root
    - group: root
    - mode: 0644
    - template: jinja
    - replace: true
    - source: salt://{{slspath}}/files/mongodb_init.conf
    - require:
      - pkg: cgroup-bin
      - file: /etc/mongodb.conf


aspera-cargod:
  cmd.run:
    - unless: dpkg -l aspera-cargod
    - name: curl -fo /tmp/aspera.deb http://download.asperasoft.com/download/sw/cargodownloader/1.6.1/aspera-cargod-1.6.1.145767-linux-64.deb && dpkg -i /tmp/aspera.deb && rm /tmp/aspera.deb

aspera:
  user.present:
    - home: /data/FOX

/etc/aspera.conf:
  file.managed:
    - user: aspera
    - group: aspera
    - mode: 600
    - contents: {{ salt['pillar.get']('aspera:config') | json }}

/etc/init/aspera.conf:
  file.managed:
    - source: salt://{{ slspath }}/upstart.conf

/var/aspera:
  file.directory:
    - user: aspera
    - group: aspera
    - mode: 755

/opt/aspera/cargod/var:
  file.symlink:
    - target: /var/aspera
    - force: True

/etc/cron.d/aspera-cron:
  file.managed:
    - source: salt://{{ slspath }}/cron

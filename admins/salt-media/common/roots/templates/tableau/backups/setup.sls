install-build-packages:
  pkg.installed:
    - pkgs:
      - 's3cmd'

/home/robot-media-tableau/.s3cfg:
  file.managed:
    - source: salt://{{ slspath }}/files/s3cfg.ini
    - user: robot-media-tableau
    - mode: 0600

/usr/bin/tableau_backup.py
  file.managed:
    - source: salt://{{ slspath }}/files/tableau_backup.py
    - user: robot-media-tableau
    - mode: 0744

create-directories:
  file.directory:
    - user: robot-media-tableau
    - mode: 755
    - makedirs: True
    - names:
      - /var/log/tableau-backups

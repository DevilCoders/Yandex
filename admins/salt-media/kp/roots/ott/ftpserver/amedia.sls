/urs/local/sbin/amedia.sh:
  file.managed:
    - user: root
    - group: root
    - mode: 0700
    - makedirs: true
    - source: salt://{{ slspath }}/files/amedia.sh
    - template: jinja
    - context:
      cred: {{ salt['pillar.get']("amedia:cred") }}

/etc/cron.d/amedia-cron:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - source: salt://{{ slspath }}/files/amedia-cron

/etc/logrotate.d/amedia-logrotate.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - source: salt://{{ slspath }}/files/amedia-logrotate.conf

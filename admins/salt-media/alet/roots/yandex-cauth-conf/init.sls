/etc/cauth/cauth.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/cauth/cauth.conf
    - user: root
    - group: root
    - mode: 0644

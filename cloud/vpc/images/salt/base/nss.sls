include:
  - .google-compute-engine-oslogin

/etc/nsswitch.conf:
  file.managed:
    - source: salt://{{ slspath }}/nss/nsswitch.conf
    - user: root
    - group: root
    - mode: 0644
    - require:
      - pkg: google-compute-engine-oslogin


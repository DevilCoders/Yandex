include:
  - google-compute-engine-oslogin

/etc/nsswitch.conf:
  file.managed:
    - source: salt://{{ slspath }}/configs/nsswitch.conf
    - user: root
    - group: root
    - mode: 0644
    - require:
      - sls: google-compute-engine-oslogin

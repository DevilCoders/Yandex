/etc/monitoring/namespace_broken.conf:
  file.managed:
    - name: /etc/monitoring/namespace_broken.conf
    - contents: |
        ignore_ns=" (avatars-pds-logo|avatars-pds-yml|avatars-tours|music-blobs|signals),"
    - user: root
    - group: root
    - mode: 644

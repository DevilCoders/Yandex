/etc/logrotate.d/s3_upload:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/s3_upload

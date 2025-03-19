/etc/logrotate.d/mail_short:
  file.managed:
    - source: salt://common/files/logrotate.d/mail_short

/etc/logrotate.d/s3_upload:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/s3_upload

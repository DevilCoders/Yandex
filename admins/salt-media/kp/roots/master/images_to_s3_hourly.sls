/usr/local/sbin/sync_last_hour_images_to_s3.sh:
  file.managed:
    - source: salt://{{slspath}}/files/usr/local/sbin/sync_last_hour_images_to_s3.sh
    - mode: 0750

/etc/cron.d/sync_last_hour_images_to_s3:
  file.managed:
    - source: salt://{{slspath}}/files/etc/cron.d/sync_last_hour_images_to_s3
    - mode: 0644

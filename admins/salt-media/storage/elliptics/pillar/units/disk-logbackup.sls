{% set unit = 'disk-logbackup' %}

disk-logbackup-cron:
  - /etc/cron.d/downloader-logbackup

disk-logbackup-sh:
  - /usr/bin/downloader-logbackup.sh
  - /usr/bin/download_counter.py
  - /usr/bin/downloads_counter_check.sh

disk-logbackup-sec:
  - /etc/downloader-logbackup-rsync.passwd



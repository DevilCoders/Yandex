/etc/cron.d/youtube-dl-updater:
  file.managed:
    - contents: |
        # /usr/bin/youtube-dl-updater.sh from pkf yandex-media-common-youtube-dl-updater
        # this cron from tamplates.youtube-dl
        0  4  *  *  *  root  /usr/bin/youtube-dl-updater.sh

{%- set env =  grains['yandex-environment'] %}

nginx-storage-config-files:
  - /etc/default/nginx
  - /etc/logrotate.d/nginx
  - /etc/monrun/conf.d/nginx.conf
  - /etc/nginx/YandexMusic/music-secure-download.pm
  - /etc/nginx/conf.d/03-tskv.conf
  - /etc/nginx/conf.d/04-disk-log.conf
  - /etc/nginx/conf.d/05-video-map.conf
  - /etc/nginx/include/common-for-upstreams.conf
  - /etc/nginx/include/devtools.conf
  - /etc/nginx/include/docker-registry.conf
  - /etc/nginx/include/jing.conf
  - /etc/nginx/include/maintenance.conf
  - /etc/nginx/include/map_allowed_origins.conf
  - /etc/nginx/include/maps-offline-caches.conf
  - /etc/nginx/include/mds-dev.conf
  - /etc/nginx/include/mediastorage-settings-disk-head.conf
  - /etc/nginx/include/mediastorage-settings-disk.conf
  - /etc/nginx/include/mediastorage-settings-head.conf
  - /etc/nginx/include/mediastorage-settings.conf
  - /etc/nginx/include/music-settings.conf
  - /etc/nginx/include/music.conf
  - /etc/nginx/include/nel.conf
  - /etc/nginx/include/pogoda.conf
  - /etc/nginx/include/rdisk-settings.conf
  - /etc/nginx/include/rdisk.conf
  - /etc/nginx/include/repo.conf
  - /etc/nginx/include/sandbox-tmp.conf
  - /etc/nginx/include/skynet.conf
  - /etc/nginx/include/spacemimic-settings-head.conf
  - /etc/nginx/include/spacemimic-settings.conf
  - /etc/nginx/include/taxi.conf
  - /etc/nginx/include/tfb.conf
  - /etc/nginx/include/tfb_testing.conf
  - /etc/nginx/include/tools_st.conf
  - /etc/nginx/include/tv.conf
  - /etc/nginx/include/zen.conf
  - /etc/nginx/nginx.conf
  - /etc/nginx/sites-enabled/10-music-storage.conf
  - /etc/nginx/sites-enabled/10-spacemimic.conf
  - /etc/nginx/ssl/https.conf
  - /etc/syslog-ng/conf-enabled/00-root.conf
  - /etc/syslog-ng/conf-enabled/nginx-access.conf
  - /etc/syslog-ng/conf-enabled/nginx-error.conf
  - /var/www/test_files/5m_ping.file

nginx-storage-dirs-root:
  - /etc/nginx/disk
  - /etc/nginx/env/include
  - /etc/nginx/include
  - /etc/nginx/sites-enabled
  - /etc/syslog-ng/conf-enabled
  - /var/log/nginx/downloader

nginx-storage-dirs-www-data:
  - /var/cache/nginx/cache/big_files
  - /var/www/test_files

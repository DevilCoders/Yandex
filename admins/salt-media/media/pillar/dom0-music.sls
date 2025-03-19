hw_watcher:
  token: {{ salt.yav.get('sec-01d56bxh7ks9t2950jzq0bhvc2[bot_token]') | json }}
  user: robot-music-admin
  monitor_logs: True

lxd:
  base:
    snap:
      enabled: True
      proxy: "http://snapd-proxy.music.dev.yandex.net:8080"
  images:
    yandex-ubuntu-trusty: https://s3.mds.yandex.net/music/salt/lxd/yandex-ubuntu-trusty.tgz

packages:
  ignored:
    - lxd
    - lxd-client

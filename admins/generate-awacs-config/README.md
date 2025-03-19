# generate-awacs-config
Generates a music-like awacs config from the simple configuration file

## config.yaml example

    global:
      project: music
      name: l7-nonprod.music.yandex.net
      sni:
        - paulus-dev.music.dev.yandex.net

    default:
      port: 80
      lo: 0.005
      hi: 0.02

    https_radio_mt_default.yml:
      group: music-test-radiofront
      host: 'radio.mt.yandex.(ru|ua|by|kz)'
      ping: 'GET /ping HTTP/1.0\nHost: radio.yandex.ru\n\n'
      section: radiotest

## Installation

    pip3 install --user https://github.yandex-team.ru/admins/generate-awacs-config/archive/master.zip
    

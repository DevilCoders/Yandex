base:
  "c:media-all":
    - match: grain
    - templates.jdowntime
    - templates.media-common
    - templates.selfdns
    - templates.graphite-sender
  "c:cult-all":
    - match: grain
    - templates.media-common
  "c:resizer-front":
    - match: grain
    - loggiver
    - resizer-front
  "c:resizer-intra-front":
    - match: grain
    - loggiver
    - resizer-intra-front
  "c:media-stable-salt":
    - match: grain
  "c:media-backup":
    - match: grain
    - backup
  "c:media-jmon":
    - match: grain
    - templates.push-client
  "c:media-stat":
    - match: grain
    - stat
  "c:media-stable-teamcity-agent":
    - match: grain
    - teamcity-agent
    - templates.repos
  "c:^media-stable-teamcity-agent-kp$":
    - match: grain_pcre
    - teamcity-agent-kp
  "c:media-stable-teamcity-agent-infra":
    - match: grain
    - teamcity-agent-infra
  "c:media-stable-mongodb":
    - match: grain
    - mongo.db
  "c:^media-dom0-music.*":
    - match: grain_pcre
    - common.monrun.salt-state.nightly
    - dom0-music
  "c:media-dom0-lxc":
    - match: grain
    - common.monrun.salt-state.nightly
    - dom0
  "c:media-ott-ftp":
    - match: grain
    - ott.ftp
  "c:media-dom0-lxd":
    - match: grain
    - dom0-lxd
  "c:media-dom0-tokk":
    - match: grain
    - dom0-tokk
  "c:media-dom0-tokk-l2":
    - match: grain
    - dom0-tokk-l2
  "c:cult-stable-backup":
    - match: grain
    - cult-stable-backup
  "c:cult-stable-logstore":
    - match: grain
    - cult-stable-logstore

  "c:media-stable-icecream":
    - match: grain
    - icecream

  'c:media-test-icecream':
    - match: grain
    - icecream
  'c:media-test-mongodb':
    - match: grain
    - mongo.db

  'c:media-dom0-alet':
    - match: grain
    - dom0-alet

  "c:media-dwh-stable-tableau":
    - match: grain
    - media-dwh-tableau

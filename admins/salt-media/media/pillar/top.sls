base:
  "c:media-all":
    - match: grain
    - jdowntime
  "c:resizer-front":
    - match: grain
    - loggiver
    - resizer-front
  "c:resizer-intra-front":
    - match: grain
    - sslyt
    - loggiver
  "c:media-jmon":
    - match: grain
    - jmon
  "c:media-stable-teamcity-agent":
    - match: grain
    - teamcity-agent
  "c:media-stable-teamcity-agent-infra":
    - match: grain
    - teamcity-agent-infra
  "c:media-stable-elastic":
    - match: grain
    - nginx
    - elastic
  "c:media-stable-mongodb":
    - match: grain
    - mongodb
  "c:media-stable-elastic-kibana":
    - match: grain
    - elastic.kibana
  "c:media-dom0-lxc":
    - match: grain
    - dom0-music
  "c:media-dom0-lxd":
    - match: grain
    - dom0-lxd
    - dom0-lxd-secrets
    - nginx
  "c:cult-stable-backup":
    - match: grain
    - cult-stable-backup
  "c:^media-stable-icecream$":
    - match: grain_pcre
    - icecream
  "c:cult-stable-logstore":
    - match: grain
    - cult-stable-logstore

  "c:media-test-mongodb":
    - match: grain
    - mongodb
  "c:media-test-icecream":
    - match: grain
    - icecream-secrets  # from media-test-secure
    - icecream
  "c:media-dom0-alet":
    - match: grain
    - dom0-alet
  "c:media-dom0-tokk":
    - match: grain
    - dom0-tokk
  "c:^media-dom0-music.*":
    - match: grain_pcre
    - dom0-music
  "c:media-dwh-stable-tableau":
    - match: grain
    - media-dwh-tableau

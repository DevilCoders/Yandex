base:
  '*':
    - templates.usr_envs
    - templates.emacs
    - templates.media-common
    - templates.graphite-sender
    - templates.conductor-agent
    - common.config-monrun-cpu-check

# ponydebugger
  'c:^kp-dev-ponydebugger$':
    - match: grain_pcre

# master
  'c:kp(-(test))?-master':
    - match: grain_pcre
    - master

# memcache
  'c:^kp-((test|load|prestable)-)?memcache$':
    - match: grain_pcre
    - templates.memcached

# static
  'c:kp-(test|load|prestable)?-?static':
    - match: grain_pcre
    - static
    - templates.yandex-free-space-watchdog-nginx-cache

# space-backup for db
  'c:kp-space-backup':
    - match: grain
    - space-backup

# space for backup data
  'space01e.kp.yandex.net':
    - backup

# new dev hosts on trusty !TODO: merge with other states, reduce dev directory
  'c:kp-dev-trusty':
    - match: grain
    ## dev.repo and dev.pkg need before templates.packages because it install percona 5.6 (need for HS)
    - dev.repo
    - dev.pkg
    # - templates.packages
    - dev
    - touch-api.secrets
    - mobile-api.secrets
    - db.common
    - db.master
    - tvmtool

# db-master
  'c:kp-((test|load)-)?db-(redb-)?master':
    - match: grain_pcre
    - db.common
    - db.master
    - db.common.mysql-monitoring-4
  'c:kp-test-db-redb-master':
    - match: grain
    - db.test-master
  'c:kp-dev-db-master':
    - match: grain
    - db.common
    - db.master
    - db.sphinx
  'c:kp-dev-db-slave':
    - match: grain
    - db.common
    - db.redb
    - db.sphinx

# db-replicas
  'c:kp-((test|load)-)?db-redb':
    - match: grain_pcre
    - db.common
    - db.common.mysql-monitoring-4
    - db.redb
    - db.redb.pt-kill
# db-sphinx
  'c:kp-((test|load)-)?db-sphinx':
    - match: grain_pcre
    - db.common
    - db.redb
    - db.sphinx
    - templates.zk_client
# db-binary
  'c:kp-db-binary':
    - match: grain
    - db.common
    - db.binary
# db-lag
  'c:kp-db-lag':
    - match: grain
    - db.common
    - db.common.mysql-monitoring-4
    - db.lag

# lxctl dom0 host
  'c:kp-dom0':
    - match: grain
    - dom0

  'c:^kp(-test)?-kino-mongo-mrs$':
    - match: grain_pcre
    - kino-mongo.mongodb_mrs

### jkp
  'c:^jkp_back(-(testing|load|unstable))?$':
    - match: grain_pcre
    - back
    - templates.yandex-free-space-watchdog-nginx-cache
  'c:^(jkp_back-testing|jkp-dev-back)$':
    - match: grain_pcre
    - back.postfix
  'c:^jkp_db_mongo-(testing|prod)-default-mongodb$':
    - match: grain_pcre
    - mongodb
    - mongo.secure
  'c:^jkp_db_mongo-(testing|prod)-default-mongodb-(\d)-backup$':
    - match: grain_pcre
    - mongo.secure
    - mongodb-backup
  'c:jkp-(test|stable)-db-mongo-tasks-mrs':
    - match: grain_pcre
    - mongodb
    - mongo.secure

### ott
  'c:ott-stable-ffmpeg':
    - match: grain
    - ott.ffmpeg
    - ott.s3
    - ott.lftp
    - ott.samba
    - ott.vncserver

  'c:ott-stable-ftp':
    - match: grain
    - ott.ftpserver

  'c:ott-(test|stable)-memcache':
    - match: grain_pcre
    - templates.memcached

  'c:ott-stable-office':
    - match: grain
    - ott.aspera-cargod
    - ott.transmission
    - templates.ffmpeg

  'c:ott-dev':
    - match: grain
    - ott.dev

  'c:^ott-stable-proxy':
    - match: grain_pcre
    - ott.proxy
    - templates.disable_rp_filter

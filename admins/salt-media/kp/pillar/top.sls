base:
  '*':
    - common.usr_envs
    - common.juggler

# environment pillar envs
  'c:kp-production-all':
    - match: grain
    - envs.production
  'c:kp-prestable-all':
    - match: grain
    - envs.prestable
  'c:kp-test-all':
    - match: grain
    - envs.testing
  'c:kp-load-all':
    - match: grain
    - envs.stress
  'c:kp-dev-all':
    - match: grain
    - envs.development

# backend
  'c:^kp-((dev|test|load|prestable)-)?backend$':
    - match: grain_pcre
    - nginx
    - backend
    - php
    - conductor-agent
    - nginx_configs

# mobile-api
  'c:^kp-((test|load|prestable)-)?mobile-api$':
    - match: grain_pcre
    - nginx
    - mobile-api
    - php
    - conductor-agent

# master
  'c:^kp-((dev|test)-)?master$':
    - match: grain_pcre
    - nginx  {# strictly in top.sls and bofore cluster's pillars #}
    - master
    - master-static-rsync
    - php
    - conductor-agent

# touch-api
  'c:^kp-((test|load|prestable)-)?touch-api$':
    - match: grain_pcre
    - nginx  {# strictly in top.sls and bofore cluster's pillars #}
    - touch-api
    - php
    - conductor-agent

# static
  'c:^kp-((test|load|prestable)-)?static$':
    - match: grain_pcre
    - nginx  {# strictly in top.sls and bofore cluster's pillars #}
    - static
    - master-static-rsync
    - conductor-agent

# memcache
  'c:^kp-((test|load|prestable)-)?memcache$':
    - match: grain_pcre
    - memcached

# db-master
  'c:kp-((test|load)-)?db-master':
    - match: grain_pcre
    - db.master
    - pt-stalk

# db-replicas
  'c:kp-((test|load)-)?db-(redb|sphinx)':
    - match: grain_pcre
    - db.redb
    - zkcli
    - sphinx
    - db.redb.pt-stalk-sphinx

# redb only
  'c:kp-((test|load)-)?db-redb':
    - match: grain_pcre
    - db.redb.pt-stalk

# heavy only
  'c:kp-((test|load)-)?db-heavy':
    - match: grain_pcre
    - db.heavy

# db-binary
  'c:kp-db-binary':
    - match: grain
    - db.binary

# db-lag
  'c:kp-db-lag':
    - match: grain
    - db.lag

# forum
  'c:kp-forum':
    - match: grain
    - forum

# space-backup for db
  'c:kp-space-backup':
    - match: grain
    - space-backup

# dev hosts on trusty
  'c:kp-dev-trusty': # dev
    - match: grain
    - memcached
    - nginx
    - backend
    - php
    - conductor-agent
    - dev
    - nginx_configs
    - db.dev-master

# dev-db
  'c:kp-dev-db-master':
    - match: grain_pcre
    - db.dev-master

  'c:kp-dev-db-slave':
    - match: grain_pcre
    - db.dev-slave

# kino-mongo
  'c:^kp(-test)?-kino-mongo-mrs$':
    - match: grain_pcre
    - kino-mongo.mongodb_mrs

### jkp
  'c:^jkp_back(-(testing|unstable))?$':
    - match: grain_pcre
    - nginx  {# strictly in top.sls and bofore cluster's pillars #}
    - back
    - conductor-agent
    - jkp_memcached
    - external_config
  # backup mongodb
  'c:jkp_db_mongo-prod-default-mongodb-(\d)-backup':
    - match: grain_pcre
    - mongo.db.backup # imported from secure repo
    - mongo.backup
    - s3cmd
  'c:jkp_db_mongo-testing-default-mongodb-(\d)-backup':
    - match: grain_pcre
    - s3cmd
    - mongo.db.backup
    - mongo.db.generic
  # default mongodb
  'c:^jkp_db_mongo-(testing|prod)-default-mongodb':
    - match: grain_pcre
    - mongo.db.generic
  # tasks mongodb
  'c:jkp-(test|stable)-db-mongo-tasks-mrs':
    - match: grain_pcre
    - mongo.db.tasks

  'c:^jkp_(testing|load)$':
    - match: grain_pcre
    - conductor-agent

### ott
  'c:ott-(test|stable)-memcache':
    - match: grain_pcre
    - ott.memcached

  'c:ott-stable-ftp':
    - match: grain
    - ott.nginx
    - ott.ftpserver

  'c:ott-stable-office':
    - match: grain
    - ott.aspera-cargod

  'c:ott-stable-ffmpeg':
    - match: grain
    - ott.ffmpeg

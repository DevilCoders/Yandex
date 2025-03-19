base:
  '*':
    - solomon_oauth

  'c:^content_(test|unstable)_all$':
    - match: grain_pcre
    - conductor-agent
  'c:^tv-(test|load|unstable|dev)$':
    - match: grain_pcre
    - conductor-agent

  'c:^content_(unstable|test|load)_tv$':
    - match: grain_pcre
    - nginx {# strictly before tv.back #}
    - tv.back
    - secrets
  'c:^(content_|tv-)(test|load)(_|-)front$':
    - match: grain_pcre
    - nginx {# strictly before tv.front #}
    - tv.front

  'c:content_test_kino':
    - match: grain
    - memcached

  'c:content_load_memcache':
    - match: grain
    - templates.memcached

  'c:^content_test_kino$':
    - match: grain_pcre
    - nginx {# strictly in top.sls and before kino.back #}
    - kino.back
    - secrets
  'c:^content_test_kino_backoffice$':
    - match: grain_pcre
    - nginx {# strictly in top.sls and before kino.backoffice #}
    - kino.backoffice
    - secrets

# dev kino(back and bko)
  'c:content-dev-kino':
    - match: grain
    - nginx
    - kino.backoffice
    - secrets

# dev tv front
  'c:tv-dev-front':
    - match: grain
    - nginx {# strictly before tv.front #}
    - tv.front

  'G@c:content_kino_back or G@c:content_prestable_kino_back':
    - match: compound
    - nginx {# strictly before kino.back and in top.sls #}
    - kino.back
    - conductor-agent
    - memcached
    - secrets
  'c:content_kino_backoffice':
    - match: grain
    - nginx {# strictly before kino.backoffice and in top.sls #}
    - kino.backoffice
    - conductor-agent
    - secrets

  'G@c:content_front or G@c:content_prestable_front':
    - match: compound
    - nginx {# strictly before tv.front and in top.sls #}
    - tv.front
    - conductor-agent
  'G@c:content_tv or G@c:content_prestable_tv':
    - match: compound
    - nginx {# strictly before tv.back and in top.sls #}
    - tv.back
    - conductor-agent
    - secrets

  'c:^tv-stable-memcache-front':
    - match: grain_pcre
    - tv.memcache.front

  'c:^content_(test_)?solr$':
    - match: grain_pcre
    - zkcli
    - s3cmd


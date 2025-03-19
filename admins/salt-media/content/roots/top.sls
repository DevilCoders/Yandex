base:
  '*':
    - templates.emacs
    - templates.usr_envs
    - templates.media-common
    - templates.fix-iface-route
    - templates.conductor-agent
  'c:content(_prestable)?_front$':
    - match: grain_pcre
    - content_front
    - templates.loggiver
    - templates.disable_rp_filter
    - templates.push-client
    - templates.nginx
    - templates.yandex-free-space-watchdog-nginx-cache
  'c:content_solr':
    - match: grain
    - content_solr
    - templates.zk_client
    - common.graphite_to_solomon
  'c:content(_prestable)?_kino_back$':
    - match: grain_pcre
    - templates.nginx
    - templates.yandex-free-space-watchdog-nginx-cache
    - templates.loggiver
    - templates.disable_rp_filter
    - templates.memcached
    - content_kino_back
    - common.secrets
  'c:content_kino_backoffice':
    - match: grain
    - templates.nginx
    - templates.yandex-free-space-watchdog-nginx-cache
    - content_kino_backoffice
    - templates.push-client
    - common.secrets
    - templates.tls_tickets
  'c:content_zookeeper':
    - match: grain
    - common.graphite_to_solomon
    - templates.disable_rp_filter

  'c:^content(_test)?_zookeeper$':
    - match: grain_pcre
    - common.graphite_to_solomon
  'c:^content_test_kino_backoffice$':
    - match: grain_pcre
    - templates.nginx
    - templates.yandex-free-space-watchdog-nginx-cache
    - content_kino_backoffice
    - templates.youtube-dl
    - templates.push-client
    - templates.tls_tickets
    - templates.certificates
    - common.graphite_to_solomon
    - common.secrets
  'c:^(content_|tv-)(test|load)(_|-)front$':
    - match: grain_pcre
    - templates.nginx
    - templates.yandex-free-space-watchdog-nginx-cache
    - templates.disable_rp_filter
    - templates.push-client
    - templates.loggiver
    - templates.certificates
    - content_front
  'c:content_test_kino$':
    - match: grain_pcre
    - templates.memcached
    - templates.nginx
    - templates.yandex-free-space-watchdog-nginx-cache
    - templates.loggiver
    - common.secrets
    - content_kino_back
  'c:content_load_memcache':
    - match: grain
    - templates.memcached
  'c:^kino-test-solr$':
    - match: grain_pcre
    - content_solr
    - templates.yandex-free-space-watchdog-nginx-cache
    - templates.zk_client
    - common.graphite_to_solomon
# unstable
  'c:content-dev-kino':
    - match: grain
    - templates.nginx
    - templates.yandex-free-space-watchdog-nginx-cache
    - content_kino_backoffice
    - templates.youtube-dl
    - templates.push-client
    - templates.tls_tickets
    - templates.certificates
    - common.graphite_to_solomon
    - common.secrets
  'c:tv-dev-front':
    - match: grain
    - templates.nginx
    - templates.yandex-free-space-watchdog-nginx-cache
    - templates.disable_rp_filter
    - templates.push-client
    - templates.tls_tickets
    - templates.certificates
    - content_front


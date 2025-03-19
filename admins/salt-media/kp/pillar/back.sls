{% set is_prod = grains["yandex-environment"] in ["production", "prestable"] %}
{% set yaenv = grains['yandex-environment'] %}

loggiver:
  lookup:
    spath: salt://common/files/etc/yandex/loggiver/loggiver.pattern

push_client:
  clean_push_client_configs: true
  domain: "kp.yandex.net"
  stats:
    {%- if is_prod %}
    - name: logbroker
      proto: tx
      ident: kinopoisk-stat
      fqdn: logbroker.yandex.net
      sszb: False
      logs:
        - file: kino-kp-api/kinopoisk-stat/collections-creation.log
          log_type: collections-creation-log
        - file: kino-kp-api/kinopoisk-stat/collections-modification.log
          log_type: collections-modification-log
        - file: kino-kp-api/kinopoisk-stat/film-voting.log
          log_type: film-voting-log
        - file: kino-kp-api/kinopoisk-stat/review-voting.log
          log_type: review-voting-log
        - file: kino-kp-api/kinopoisk-stat/watchlist-modification.log
          log_type: watchlist-modification-log
    - name: logbroker-users-action
      fqdn: logbroker.yandex.net
      port: default
      proto: rt
      sszb: False
      server_lite: False
      logger: { remote: 0 }
      ident: kp-master
      logs:
        - file: kino-kp-api/users/actions.tskv.log
          log_type: kp-api-user-action-log
    {%- endif %}
  logs:
    - file: nginx/access.log
    - file: nginx/error.log
    - file: kino-kp-api/http-requests/latency.log
    - file: kino-kp-api/app/app.log
    - file: kino-kp-api/mongo/operation.log
    - file: kino-kp-api/users/actions.tskv.log

nginx:
  lookup:
    log_params:
      name: 'access-log'
      access_log_name: access.log
      custom_fields:
        - "http_clientid=$http_clientid"

graphite-sender:
  sender:
    max_metrics: 10000


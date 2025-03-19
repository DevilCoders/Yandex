/etc/default/juggler-client:
  file.managed:
    - mode: 644
    - user: root
    - group: root
    - contents: |
        JUGGLER_USER="root"
        JUGGLER_ROOT="/etc/yandex/juggler-client"

/etc/yandex/juggler-client/etc/client.conf:
  file.managed:
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - contents: |
        [client]
        config_url=http://juggler-api.search.yandex.net
        targets=AUTO
        batch_delay=5
        check_bundles=wall-e-checks-bundle

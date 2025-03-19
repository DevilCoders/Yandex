#!/usr/bin/env bash
LOGS=/var/log/mdb-image-releaser
timeout 30m /opt/yandex/mdb-image-releaser/bin/mdb-image-releaser {{ mode }} release \
    --config-path /opt/yandex/mdb-image-releaser \
    --name {{ name }} \
{% if no_check %}
    --no-check \
{% else %}
    --check-host {{ check_host }} \
    --check-service {{ check_service }} \
    --stable {{ salt.pillar.get('data:image-releaser:stable_duration', '4h') }} \
{% endif %}
{% if mode == 'compute' %}
    --folder {{ folder }} \
    --pool-size {{ pool_size }} \
    --cleanup \
    --keep-images {{ keep_images }} \
    --os {{ os }} \
{% if product_ids -%}
    --product-ids {{ product_ids|join(',') }} \
{% endif %}
{% endif %}
    --logshowall >> $LOGS/stdout.log 2>> $LOGS/stderr.log

echo "$?" > /var/run/mdb-image-releaser/last-exit-status

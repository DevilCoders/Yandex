ARG fluent_bit_version=1.9.3
FROM \
    cr.yandex/crpsjg1coh47p81vh2lc/fluent-bit-plugin-yandex:v2.0.2-fluent-bit-${fluent_bit_version} \
    AS fluentbit-stage

COPY fluent-bit.conf fluent-bit/etc/fluent-bit.conf

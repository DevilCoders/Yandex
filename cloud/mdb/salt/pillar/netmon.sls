{% set endpoints = salt.grains.filter_by({
    'vla': {
        'yp_sd_cluster_name': 'vla',
        'yp_sd_endpoint_set_id': 'yp-netmon-aggregator-noc-sla-vla',
        'provisioning_url': 'http://f34zwx2zzlgihx47.vla.yp-c.yandex.net',
        'noc_sla_url': 'http://f34zwx2zzlgihx47.vla.yp-c.yandex.net,http://yp-netmon-aggregator-noc-sla-1.vla.yp-c.yandex.net,http://yp-netmon-aggregator-noc-sla-2.vla.yp-c.yandex.net',
    },
    'sas': {
        'yp_sd_cluster_name': 'sas',
        'yp_sd_endpoint_set_id': 'yp-netmon-aggregator-noc-sla-sas',
        'provisioning_url': 'http://yp-netmon-aggregator-noc-sla-sas-11.sas.yp-c.yandex.net',
        'noc_sla_url': 'http://yp-netmon-aggregator-noc-sla-sas-11.sas.yp-c.yandex.net,http://yp-netmon-aggregator-noc-sla-sas-5.sas.yp-c.yandex.net,http://yp-netmon-aggregator-noc-sla-sas-6.sas.yp-c.yandex.net',
    },
    'man': {
        'yp_sd_cluster_name': 'man',
        'yp_sd_endpoint_set_id': 'yp-netmon-aggregator-noc-sla-man',
        'provisioning_url': 'http://yp-netmon-aggregator-noc-sla-man-6.man.yp-c.yandex.net',
        'noc_sla_url': 'http://yp-netmon-aggregator-noc-sla-man-6.man.yp-c.yandex.net,http://yp-netmon-aggregator-noc-sla-man-7.man.yp-c.yandex.net,http://yp-netmon-aggregator-noc-sla-man-8.man.yp-c.yandex.net',
    },
    'iva': {
        'yp_sd_cluster_name': 'iva',
        'yp_sd_endpoint_set_id': 'yp-netmon-aggregator-noc-sla-iva',
        'provisioning_url': 'http://yp-netmon-aggregator-noc-sla-iva-1.iva.yp-c.yandex.net',
        'noc_sla_url': 'http://yp-netmon-aggregator-noc-sla-iva-1.iva.yp-c.yandex.net,http://yp-netmon-aggregator-noc-sla-iva-2.iva.yp-c.yandex.net,http://yp-netmon-aggregator-noc-sla-iva-3.iva.yp-c.yandex.net',
    },
    'myt': {
        'yp_sd_cluster_name': 'myt',
        'yp_sd_endpoint_set_id': 'yp-netmon-aggregator-noc-sla-myt',
        'provisioning_url': 'http://yp-netmon-aggregator-noc-sla-myt-1.myt.yp-c.yandex.net',
        'noc_sla_url': 'http://yp-netmon-aggregator-noc-sla-myt-1.myt.yp-c.yandex.net,http://yp-netmon-aggregator-noc-sla-myt-2.myt.yp-c.yandex.net,http://yp-netmon-aggregator-noc-sla-myt-4.myt.yp-c.yandex.net',
    },
}, 'ya:short_dc' ) %}
# https://st.yandex-team.ru/NETMONSUPPORT-238#618a936d1b5f1130711179d2
data:
    netmon:
        endpoints: {{ endpoints | tojson }}

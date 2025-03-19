{% set config_site = {} %}

{% do config_site.update({'capacity-scheduler': salt['grains.filter_by']({
    'Debian': {
        'yarn.scheduler.capacity.maximum-am-resource-percent': '0.25',
    },
}, merge=salt['pillar.get']('data:properties:capacity-scheduler'))}) %}

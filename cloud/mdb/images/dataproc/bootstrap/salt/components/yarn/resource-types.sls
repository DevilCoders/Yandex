{% set config_site = {} %}

{% do config_site.update({'resource-types': salt['grains.filter_by']({
    'Debian': {
        'yarn.resource-types': '',
    },
}, merge=salt['pillar.get']('data:properties:resource-types'))}) %}

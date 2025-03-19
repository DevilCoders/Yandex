{% set config_site = {} %}

{% do config_site.update({'node-resources': salt['grains.filter_by']({
    'Debian': {
    },
}, merge=salt['pillar.get']('data:properties:node-resources'))}) %}

{% set config_site = {} %}

{% do config_site.update({'hbase-policy': salt['grains.filter_by']({
    'Debian': {
      'security.client.protocol.acl': '*'
    },
}, merge=salt['pillar.get']('data:properties:hbase-policy'))}) %}

{% set config_site = {} %}

{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}

{% do config_site.update({'hiveserver2': salt['grains.filter_by']({
    'Debian': {
        'hive.server2.metrics.enabled': 'true',
    }
}, merge=salt['pillar.get']('data:properties:hiveserver2'))}) %}

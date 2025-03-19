{% set config_site = {} %}

{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}

{% set cores = salt['grains.get']('num_cpus') %}
{% set memory_mb = ((salt['grains.get']('mem_total') / 1024) | int) * 1024  %}

{% do config_site.update({'hbase': salt['grains.filter_by']({
    'Debian': {
        'hbase.cluster.distributed': 'true',
        'hbase.zookeeper.quorum':  masternode,
        'hbase.rootdir': 'hdfs://' + masternode  + ':8020/user/hbase',
        'zookeeper.recovery.retry': 60,
        'zookeeper.recovery.retry.intervalmill': 5000,
        'dfs.support.append': 'true',
        'hbase.rest.port': '8070'
    },
}, merge=salt['pillar.get']('data:properties:hbase'))}) %}

{% set config_site = {} %}

{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}
{% set masternode_web = '0.0.0.0' if 'masternode' in roles else masternode %}

{% set fqdn = salt['grains.get']('dataproc:fqdn') %}

{% set config_site = {
    'dfs.namenode.rpc-address': masternode + ':8020',
    'dfs.namenode.http-address': masternode_web + ':9870',
    'dfs.namenode.https-address': masternode_web + ':9871',

    'dfs.namenode.name.dir': 'file:///hadoop/dfs/name',
    'dfs.namenode.handler.count': 64,
    'dfs.namenode.checkpoint.dir': 'file:///hadoop/dfs/namesecondary',

    'dfs.hosts.exclude': hadoop_config_path + '/dfs.exclude',

    'dfs.replication': 1,
    'dfs.permissions.supergroup': 'hadoop',
    'dfs.blocksize': 268435456,
    'dfs.datanode.data.dir': 'file:///hadoop/dfs/data',
    'dfs.datanode.data.dir.perm': 700
    }
%}

{% if salt['ydputils.is_datanode']() %}
    {% do config_site.update({'dfs.datanode.http.address': fqdn + ':9864'}) %}
    {% do config_site.update({'dfs.datanode.https.address': fqdn + ':9865'}) %}
    {% do config_site.update({'dfs.datanode.address': fqdn + ':9866'}) %}
    {% do config_site.update({'dfs.datanode.ipc.address': fqdn + ':9867'}) %}
{% endif %}

{% for property, value in salt['pillar.get']('data:properties:hdfs').iteritems() %}
    {% do config_site.update({property: value}) %}
{% endfor %}

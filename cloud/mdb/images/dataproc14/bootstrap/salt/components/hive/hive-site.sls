{% set config_site = {} %}

{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}

{% set hive_version = m.version('hive') %}

{% set settings = salt['pillar.get']('data:settings') %}
{% set host = 'localhost' %}
{% set jdbc_url = 'jdbc:postgresql://' + host + ':5432/' + settings['hive_db_name'] %}
{% set jdbc_driver = settings['hive_driver'] %}
{% set jdbc_user = settings['hive_db_user'] %}
{% set jdbc_password = settings['hive_db_password'] %}

{% do config_site.update({'hive': salt['grains.filter_by']({
    'Debian': {
        'javax.jdo.option.ConnectionURL': jdbc_url,
        'javax.jdo.option.ConnectionDriverName': jdbc_driver,
        'javax.jdo.option.ConnectionUserName': jdbc_user,
        'javax.jdo.option.ConnectionPassword': jdbc_password,
        'hive.server2.metrics.enabled': 'true',
        'hive.metastore.connect.retries': 60,
        'hive.metastore.uris': 'thrift://' + masternode+ ':9083',
        'datanucleus.autoCreateSchema': 'false',
        'datanucleus.fixedDatastore': 'true',
        'datanucleus.autoStartMechanism': 'schemaTable',
        'hive.metastore.schema.verification': 'true'
    }
}, merge=salt['pillar.get']('data:properties:hive'))}) %}

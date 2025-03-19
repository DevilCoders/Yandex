{% set config_site = {} %}

{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}

{% set settings = salt['pillar.get']('data:settings') %}
{% set oozie = settings['oozie'] %}
{% set host = 'localhost' %}
{% set jdbc_url = 'jdbc:postgresql://' + host + ':5432/' + oozie['db_name'] %}
{% set jdbc_driver = oozie['driver'] %}
{% set jdbc_user = oozie['db_user'] %}
{% set jdbc_password = oozie['db_password'] %}

{% do config_site.update({'oozie': salt['grains.filter_by']({
    'Debian': {
        'oozie.service.HadoopAccessorService.supported.filesystems': 'hdfs,s3,s3n',
        'oozie.http.hostname': masternode,
        'oozie.service.WorkflowAppService.system.libpath': '/user/${user.name}/share/lib',
        'oozie.system.id': '${user.name}',
        'oozie.actions.default.name-node': ' ',
        'oozie.service.HadoopAccessorService.hadoop.configurations': '*=/etc/hadoop/conf',
        'oozie.db.schema.name': 'oozie',
        'oozie.service.JPAService.create.db.schema': 'false',
        'oozie.service.JPAService.validate.db.connection': 'false',
        'oozie.service.JPAService.jdbc.driver': jdbc_driver,
        'oozie.service.JPAService.jdbc.url': jdbc_url,
        'oozie.service.JPAService.jdbc.username': jdbc_user,
        'oozie.service.JPAService.jdbc.password': jdbc_password,
        'oozie.service.JPAService.pool.max.active.conn': '10',
        'oozie.zookeeper.connection.string': masternode + ':2181',
        'oozie.services.ext': 'org.apache.oozie.service.MetricsInstrumentationService',
        'oozie.jmx_monitoring.enable': 'true'
    },
}, merge=salt['pillar.get']('data:properties:oozie'))}) %}

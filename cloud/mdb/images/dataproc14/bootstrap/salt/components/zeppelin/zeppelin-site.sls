{% set config_site = {} %}

{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}

{% set cores = salt['grains.get']('num_cpus') %}
{% set memory_mb = ((salt['grains.get']('mem_total') / 1024) | int) * 1024  %}

{% do config_site.update({'zeppelin': salt['grains.filter_by']({
    'Debian': {
        'zeppelin.server.addr': '0.0.0.0',
        'zeppelin.server.port': '8890',
        'zeppelin.server.context.path': '/',
        'zeppelin.war.tempdir': 'webapps',
        'zeppelin.server.kerberos.principal': '',
        'zeppelin.notebook.storage': 'org.apache.zeppelin.notebook.repo.GitNotebookRepo, org.apache.zeppelin.notebook.repo.zeppelinhub.ZeppelinHubRepo',
        'zeppelin.notebook.one.way.sync': 'false',
        'zeppelin.interpreter.dir': 'interpreter',
        'zeppelin.interpreter.localRepo': 'local-repo',
        'zeppelin.interpreter.dep.mvnRepo': 'http://repo1.maven.org/maven2/',
        'zeppelin.dep.localrepo': 'local-repo',
        'zeppelin.helium.node.installer.url': 'https://nodejs.org/dist/',
        'zeppelin.helium.npm.installer.url': 'http://registry.npmjs.org/',
        'zeppelin.helium.yarnpkg.installer.url': 'https://github.com/yarnpkg/yarn/releases/download/',
        'zeppelin.interpreters': 'org.apache.zeppelin.spark.SparkInterpreter,org.apache.zeppelin.spark.PySparkInterpreter,org.apache.zeppelin.rinterpreter.RRepl,org.apache.zeppelin.rinterpreter.KnitR,org.apache.zeppelin.spark.SparkRInterpreter,org.apache.zeppelin.spark.SparkSqlInterpreter,org.apache.zeppelin.spark.DepInterpreter,org.apache.zeppelin.markdown.Markdown,org.apache.zeppelin.angular.AngularInterpreter,org.apache.zeppelin.shell.ShellInterpreter,org.apache.zeppelin.file.HDFSFileInterpreter,org.apache.zeppelin.flink.FlinkInterpreter,,org.apache.zeppelin.python.PythonInterpreter,org.apache.zeppelin.python.PythonInterpreterPandasSql,org.apache.zeppelin.python.PythonCondaInterpreter,org.apache.zeppelin.python.PythonDockerInterpreter,org.apache.zeppelin.lens.LensInterpreter,org.apache.zeppelin.ignite.IgniteInterpreter,org.apache.zeppelin.ignite.IgniteSqlInterpreter,org.apache.zeppelin.cassandra.CassandraInterpreter,org.apache.zeppelin.geode.GeodeOqlInterpreter,org.apache.zeppelin.jdbc.JDBCInterpreter,org.apache.zeppelin.kylin.KylinInterpreter,org.apache.zeppelin.elasticsearch.ElasticsearchInterpreter,org.apache.zeppelin.scalding.ScaldingInterpreter,org.apache.zeppelin.alluxio.AlluxioInterpreter,org.apache.zeppelin.hbase.HbaseInterpreter,org.apache.zeppelin.livy.LivySparkInterpreter,org.apache.zeppelin.livy.LivyPySparkInterpreter,org.apache.zeppelin.livy.LivyPySpark3Interpreter,org.apache.zeppelin.livy.LivySparkRInterpreter,org.apache.zeppelin.livy.LivySparkSQLInterpreter,org.apache.zeppelin.bigquery.BigQueryInterpreter,org.apache.zeppelin.beam.BeamInterpreter,org.apache.zeppelin.pig.PigInterpreter,org.apache.zeppelin.pig.PigQueryInterpreter,org.apache.zeppelin.scio.ScioInterpreter,org.apache.zeppelin.groovy.GroovyInterpreter',
        'zeppelin.interpreter.group.order': 'spark,md,angular,sh,livy,alluxio,file,psql,flink,python,ignite,lens,cassandra,geode,kylin,elasticsearch,scalding,jdbc,hbase,bigquery,beam,groovy',
        'zeppelin.server.allowed.origins': '*'
    },
}, merge=salt['pillar.get']('data:properties:zeppelin'))}) %}

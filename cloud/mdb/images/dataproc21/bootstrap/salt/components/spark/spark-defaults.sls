{% set services = salt['pillar.get']('data:services') %}
{% set masternode = salt['ydputils.get_masternodes']()[0] %}

{% set event_log_dir = salt['ydp-fs.fs_url_for_path']('/var/log/spark/apps', allow_local=True) %}
{% set history_log_dir = salt['ydp-fs.fs_url_for_path']('/var/log/spark/apps', allow_local=True) %}
{% set spark_history_server_port = 18080 %}
{% set spark_history_server = masternode ~ ':' ~ spark_history_server_port %}
{% set spark_driver_cores = salt['ydputils.get_spark_driver_cores']() %}
{% set spark_executor_cores = salt['ydputils.get_spark_executor_cores']() %}
{% set spark_executor_memory = salt['ydputils.get_spark_executor_memory_mb']() ~ "m" %}
{% set spark_driver_memory = salt['ydputils.get_spark_driver_mem_mb']() ~ "m" %}
{% set spark_driver_result_size_memory = salt['ydputils.get_spark_driver_result_size_mem_mb']() ~ "m" %}

{% set config = {
  'spark.master': 'yarn',
  'spark.eventLog.enabled': 'true',
  'spark.submit.deployMode': 'client',
  'spark.eventLog.dir': event_log_dir,
  'spark.history.fs.logDirectory': history_log_dir,
  'spark.history.fs.cleaner.enabled': 'true',
  'spark.history.fs.cleaner.interval': '1d',
  'spark.history.fs.cleaner.maxAge': '7d',
  'spark.history.ui.port': spark_history_server_port,
  'spark.yarn.historyServer.address': spark_history_server,
  'spark.shuffle.service.enabled': 'true',

  'spark.dynamicAllocation.enabled': 'true',
  'spark.dynamicAllocation.timeout': '1h',
  'spark.dynamicAllocation.minExecutors': 1,
  'spark.dynamicAllocation.maxExecutors': 8192,

  'spark.blacklist.decommissioning.enabled': 'true',
  'spark.blacklist.decommissioning.timeout': '1h',

  'spark.resourceManager.cleanupExpiredHost': 'true',
  'spark.stage.attempt.ignoreOnDecommissionFetchFailure': true,
  'spark.decommissioning.timeout.threshold': 20,

  'spark.hadoop.yarn.timeline-service.enabled': 'false',
  'spark.yarn.appMasterEnv.SPARK_PUBLIC_DNS': '$(hostname -f)',
  'spark.files.fetchFailure.unRegisterOutputOnHost': true,

  'spark.executor.memory': spark_executor_memory,
  'spark.executor.cores': spark_executor_cores,
  'spark.yarn.am.memory': spark_driver_memory,
  'spark.driver.cores': spark_driver_cores,
  'spark.driver.memory': spark_driver_memory,
  'spark.driver.maxResultSize': spark_driver_result_size_memory,
  'spark.driver.extraJavaOptions': '-XX:+UseConcMarkSweepGC -XX:CMSInitiatingOccupancyFraction=70 -XX:MaxHeapFreeRatio=70 -XX:+CMSClassUnloadingEnabled -XX:OnOutOfMemoryError=\'kill -9 %p\'',
  'spark.executor.extraJavaOptions': '-verbose:gc -XX:+PrintGCDetails -XX:+PrintGCDateStamps -XX:+UseConcMarkSweepGC -XX:CMSInitiatingOccupancyFraction=70 -XX:MaxHeapFreeRatio=70 -XX:+CMSClassUnloadingEnabled -XX:OnOutOfMemoryError=\'kill -9 %p\'',

  'spark.sql.sources.commitProtocolClass': 'org.apache.spark.internal.io.cloud.PathOutputCommitProtocol',
  'spark.sql.parquet.output.committer.class': 'org.apache.spark.internal.io.cloud.BindingParquetOutputCommitter',

  'spark.sql.cbo.enable': 'true',
  'spark.yarn.jars': 'local:/usr/lib/spark/jars/*,local:/usr/lib/spark/external/lib/*'
} %}

{% if 'hive' in services %}
{% do config.update({
  'spark.sql.catalogImplementation': 'hive',
  'spark.sql.warehouse.dir': salt['ydputils.get_hive_warehouse_path'](),
}
)%}
{% endif %}
{% do config.update(salt['pillar.get']('data:properties:spark', {})) %}

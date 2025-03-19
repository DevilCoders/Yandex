import json
from dataclasses import dataclass
from cloud.dwh.lms.exceptions import LMSException
from cloud.dwh.lms.utils import import_string
from cloud.dwh.lms.utils.log.logging_mixin import LoggingMixin

CONN_TYPE_TO_HOOK = {
    "postgres": ("cloud.dwh.lms.hooks.postgres_hook.PostgresHook", "postgres_conn_id"),
}


@dataclass
class Connection(LoggingMixin):
    """
    Placeholder to store information about different database instances
    connection information. The idea here is that scripts use references to
    database instances (conn_id) instead of hard coding hostname, logins and
    passwords when using operators or hooks.
    """
    conn_id: str
    conn_type: str
    host: str
    port: int
    login: str
    schema: str
    password: str
    extra: str

    _types = [
        ('docker', 'Docker Registry'),
        ('elasticsearch', 'Elasticsearch'),
        ('exasol', 'Exasol'),
        ('facebook_social', 'Facebook Social'),
        ('fs', 'File (path)'),
        ('ftp', 'FTP'),
        ('google_cloud_platform', 'Google Cloud Platform'),
        ('hdfs', 'HDFS'),
        ('http', 'HTTP'),
        ('pig_cli', 'Pig Client Wrapper'),
        ('hive_cli', 'Hive Client Wrapper'),
        ('hive_metastore', 'Hive Metastore Thrift'),
        ('hiveserver2', 'Hive Server 2 Thrift'),
        ('jdbc', 'JDBC Connection'),
        ('odbc', 'ODBC Connection'),
        ('jenkins', 'Jenkins'),
        ('mysql', 'MySQL'),
        ('postgres', 'Postgres'),
        ('oracle', 'Oracle'),
        ('vertica', 'Vertica'),
        ('presto', 'Presto'),
        ('s3', 'S3'),
        ('samba', 'Samba'),
        ('sqlite', 'Sqlite'),
        ('ssh', 'SSH'),
        ('cloudant', 'IBM Cloudant'),
        ('mssql', 'Microsoft SQL Server'),
        ('mesos_framework-id', 'Mesos Framework ID'),
        ('jira', 'JIRA'),
        ('redis', 'Redis'),
        ('wasb', 'Azure Blob Storage'),
        ('databricks', 'Databricks'),
        ('aws', 'Amazon Web Services'),
        ('emr', 'Elastic MapReduce'),
        ('snowflake', 'Snowflake'),
        ('segment', 'Segment'),
        ('sqoop', 'Sqoop'),
        ('azure_batch', 'Azure Batch Service'),
        ('azure_data_lake', 'Azure Data Lake'),
        ('azure_container_instances', 'Azure Container Instances'),
        ('azure_cosmos', 'Azure CosmosDB'),
        ('azure_data_explorer', 'Azure Data Explorer'),
        ('cassandra', 'Cassandra'),
        ('qubole', 'Qubole'),
        ('mongo', 'MongoDB'),
        ('gcpcloudsql', 'Google Cloud SQL'),
        ('grpc', 'GRPC Connection'),
        ('yandexcloud', 'Yandex Cloud'),
        ('livy', 'Apache Livy'),
        ('tableau', 'Tableau'),
        ('kubernetes', 'Kubernetes cluster Connection'),
        ('spark', 'Spark'),
    ]

    def get_hook(self):
        hook_class_name, conn_id_param = CONN_TYPE_TO_HOOK.get(self.conn_type, (None, None))
        if not hook_class_name:
            raise LMSException('Unknown hook type "{}"'.format(self.conn_type))
        hook_class = import_string(hook_class_name)
        return hook_class(**{conn_id_param: self.conn_id})

    def __repr__(self):
        return self.conn_id

    def log_info(self):
        return ("yav_secret_version: {}. Host: {}, Port: {}, Schema: {}, "
                "Login: {}, Password: {}, extra: {}".
                format(self.conn_id,
                       self.host,
                       self.port,
                       self.schema,
                       self.login,
                       "XXXXXXXX" if self.password else None,
                       "XXXXXXXX" if self.extra else None))

    def debug_info(self):
        return ("id: {}. Host: {}, Port: {}, Schema: {}, "
                "Login: {}, Password: {}, extra: {}".
                format(self.conn_id,
                       self.host,
                       self.port,
                       self.schema,
                       self.login,
                       "XXXXXXXX" if self.password else None,
                       self.extra))

    @property
    def extra_dejson(self):
        """Returns the extra property by deserializing json."""
        obj = {}
        if self.extra:
            try:
                obj = json.loads(self.extra)
            except Exception as e:
                self.log.exception(e)
                self.log.error("Failed parsing the json for conn_id %s", self.conn_id)

        return obj

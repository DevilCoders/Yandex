import re
import json

from django.conf import settings
import django.db
import django.contrib.postgres.fields as pg_fields

import cloud.mdb.backstage.contrib.dictdiffer as dictdiffer

import cloud.mdb.backstage.lib.helpers as mod_helpers

import cloud.mdb.backstage.apps.deploy.models as deploy_models


class WorkerTaskAction:
    CANCEL = ('cancel', 'Cancel')
    RESTART = ('restart', 'Restart')
    REJECT = ('reject', 'Reject')
    FINISH = ('finish', 'Finish')

    all =[
        CANCEL,
        RESTART,
        REJECT,
        FINISH,
    ]

    map = {
        CANCEL[0]: CANCEL,
        RESTART[0]: RESTART,
        REJECT[0]: REJECT,
        FINISH[0]: FINISH,
    }


class BackupStatus:
    PLANNED = ('PLANNED', 'Planned')
    CREATING = ('CREATING', 'Creating')
    DONE = ('DONE', 'Done')
    OBSOLETE = ('OBSOLETE', 'Obsolete')
    DELETING = ('DELETING', 'Deleting')
    DELETED = ('DELETED', 'Deleted')
    CREATE_ERROR = ('CREATE-ERROR', 'Create: error')
    DELETE_ERROR = ('DELETE-ERROR', 'Delete: error')

    all = [
        PLANNED,
        CREATING,
        DONE,
        OBSOLETE,
        DELETING,
        DELETED,
        CREATE_ERROR,
        DELETE_ERROR,
    ]


class BackupInitiator:
    SCHEDULE = ('SCHEDULE', 'Schedule')
    USER = ('USER', 'User')

    all = [
        SCHEDULE,
        USER,
    ]


class BackupMethod:
    FULL = ('FULL', 'Full')
    INCREMENTAL = ('INCREMENTAL', 'Incremental')

    all = [
        FULL,
        INCREMENTAL,
    ]


class ClusterType:
    POSTGRESQL_CLUSTER = ('postgresql_cluster', 'PostgreSQL')
    PGAAS_PROXY = ('pgaas-proxy', 'PGAAS Proxy')
    ZOOKEEPER = ('zookeeper', 'ZooKeeper')
    CLICKHOUSE_CLUSTER = ('clickhouse_cluster', 'ClickHouse')
    MONGODB_CLUSTER = ('mongodb_cluster', 'MongoDB')
    REDIS_CLUSTER = ('redis_cluster', 'Redis')
    MYSQL_CLUSTER = ('mysql_cluster', 'MySQL')
    GREENPLUM_CLUSTER = ('greenplum_cluster', 'Greenplum')
    SQLSERVER_CLUSTER = ('sqlserver_cluster', 'SQL Server')
    HADOOP_CLUSTER = ('hadoop_cluster', 'Hadoop')
    KAFKA_CLUSTER = ('kafka_cluster', 'Kafka')
    ELASTICSEARCH_CLUSTER = ('elasticsearch_cluster', 'ElasticSearch')

    all = [
        POSTGRESQL_CLUSTER,
        PGAAS_PROXY,
        ZOOKEEPER,
        CLICKHOUSE_CLUSTER,
        MONGODB_CLUSTER,
        REDIS_CLUSTER,
        MYSQL_CLUSTER,
        GREENPLUM_CLUSTER,
        SQLSERVER_CLUSTER,
        HADOOP_CLUSTER,
        KAFKA_CLUSTER,
        ELASTICSEARCH_CLUSTER,
    ]
    map = {
        POSTGRESQL_CLUSTER[0]: POSTGRESQL_CLUSTER,
        PGAAS_PROXY[0]: PGAAS_PROXY,
        ZOOKEEPER[0]: ZOOKEEPER,
        CLICKHOUSE_CLUSTER[0]: CLICKHOUSE_CLUSTER,
        REDIS_CLUSTER[0]: REDIS_CLUSTER,
        MYSQL_CLUSTER[0]: MYSQL_CLUSTER,
        GREENPLUM_CLUSTER[0]: GREENPLUM_CLUSTER,
        MONGODB_CLUSTER[0]: MONGODB_CLUSTER,
        SQLSERVER_CLUSTER[0]: SQLSERVER_CLUSTER,
        HADOOP_CLUSTER[0]: HADOOP_CLUSTER,
        KAFKA_CLUSTER[0]: KAFKA_CLUSTER,
        ELASTICSEARCH_CLUSTER[0]: ELASTICSEARCH_CLUSTER,
    }


class BooleanChoices:
    TRUE = (True, 'True')
    FALSE = (False, 'False')

    all = [
        TRUE,
        FALSE,
    ]


class EnvType:
    DEV = ('dev', 'Dev')
    LOAD = ('load', 'Load')
    QA = ('qa', 'Qa')
    PROD = ('prod', 'Prod')
    COMPUTE_PROD = ('compute-prod', 'Compute-prod')

    all = [
        DEV,
        LOAD,
        QA,
        PROD,
        COMPUTE_PROD,
    ]


class ClusterStatus:
    CREATING = ('CREATING', 'Creating')
    CREATE_ERROR = ('CREATE-ERROR', 'Create: error')
    RUNNING = ('RUNNING', 'Running')
    MODIFYING = ('MODIFYING', 'Modifying')
    MODIFY_ERROR = ('MODIFY-ERROR', 'Modify: error')
    STOPPING = ('STOPPING', 'Stopping')
    STOP_ERROR = ('STOP-ERROR', 'Stop: error')
    STOPPED = ('STOPPED', 'Stopped')
    STARTING = ('STARTING', 'Starting')
    START_ERROR = ('START-ERROR', 'Start: error')
    DELETING = ('DELETING', 'Deleting')
    DELETE_ERROR = ('DELETE-ERROR', 'Delete: error')
    DELETED = ('DELETED', 'Deleted')
    PURGING = ('PURGING', 'Purging')
    PURGE_ERROR = ('PURGE-ERROR', 'Purge: error')
    PURGED = ('PURGED', 'Purged')
    METADATA_DELETING = ('METADATA-DELETING', 'Metadata: deleting')
    METADATA_DELETE_ERROR = ('METADATA-DELETE-ERROR', 'Metadata delete: error')
    METADATA_DELETED = ('METADATA-DELETED', 'Metadata: deleted')

    all = [
        CREATING,
        CREATE_ERROR,
        RUNNING,
        MODIFYING,
        MODIFY_ERROR,
        STOPPING,
        STOP_ERROR,
        STOPPED,
        STARTING,
        START_ERROR,
        DELETING,
        DELETE_ERROR,
        DELETED,
        PURGING,
        PURGE_ERROR,
        PURGED,
        METADATA_DELETING,
        METADATA_DELETE_ERROR,
        METADATA_DELETED,
    ]
    error = [
        CREATE_ERROR,
        STOP_ERROR,
        START_ERROR,
        MODIFY_ERROR,
        DELETE_ERROR,
        PURGE_ERROR,
        METADATA_DELETE_ERROR,
    ]

    not_removed = [
        CREATING,
        CREATE_ERROR,
        RUNNING,
        MODIFYING,
        MODIFY_ERROR,
        STOPPING,
        STOP_ERROR,
        STOPPED,
        STARTING,
        START_ERROR,
        DELETING,
        DELETE_ERROR,
        PURGING,
        PURGE_ERROR,
        METADATA_DELETING,
        METADATA_DELETE_ERROR,
    ]

    active = [
        CREATING,
        CREATE_ERROR,
        RUNNING,
        MODIFYING,
        MODIFY_ERROR,
        STARTING,
        START_ERROR,
    ]


class RoleTypeChoices:
    POSTGRESQL_CLUSTER = ('postgresql_cluster', 'Postgresql_cluster')
    CLICKHOUSE_CLUSTER = ('clickhouse_cluster', 'Clickhouse_cluster')
    PGAAS_PROXY = ('pgaas-proxy', 'Pgaas-proxy')
    MONGODB_CLUSTER = ('mongodb_cluster', 'Mongodb_cluster')
    ZK = ('zk', 'Zk')
    REDIS_CLUSTER = ('redis_cluster', 'Redis_cluster')
    MYSQL_CLUSTER = ('mysql_cluster', 'Mysql_cluster')
    SQLSERVER_CLUSTER = ('sqlserver_cluster', 'Sqlserver_cluster')
    MONGODB_CLUSTER_MONGOD = ('mongodb_cluster.mongod', 'Mongodb_cluster.mongod')
    MONGODB_CLUSTER_MONGOCFG = ('mongodb_cluster.mongocfg', 'Mongodb_cluster.mongocfg')
    MONGODB_CLUSTER_MONGOS = ('mongodb_cluster.mongos', 'Mongodb_cluster.mongos')
    MONGODB_CLUSTER_MONGOINFRA = ('mongodb_cluster.mongoinfra', 'Mongodb_cluster.mongoinfra')
    HADOOP_CLUSTER_MASTERNODE = ('hadoop_cluster.masternode', 'Hadoop_cluster.masternode')
    HADOOP_CLUSTER_DATANODE = ('hadoop_cluster.datanode', 'Hadoop_cluster.datanode')
    HADOOP_CLUSTER_COMPUTENODE = ('hadoop_cluster.computenode', 'Hadoop_cluster.computenode')
    KAFKA_CLUSTER = ('kafka_cluster', 'Kafka_cluster')
    ELASTICSEARCH_CLUSTER_DATANODE = ('elasticsearch_cluster.datanode', 'Elasticsearch_cluster.datanode')
    ELASTICSEARCH_CLUSTER_MASTERNODE = ('elasticsearch_cluster.masternode', 'Elasticsearch_cluster.masternode')

    all = [
        POSTGRESQL_CLUSTER,
        CLICKHOUSE_CLUSTER,
        PGAAS_PROXY,
        MONGODB_CLUSTER,
        ZK,
        REDIS_CLUSTER,
        MYSQL_CLUSTER,
        SQLSERVER_CLUSTER,
        MONGODB_CLUSTER_MONGOD,
        MONGODB_CLUSTER_MONGOCFG,
        MONGODB_CLUSTER_MONGOS,
        MONGODB_CLUSTER_MONGOINFRA,
        HADOOP_CLUSTER_MASTERNODE,
        HADOOP_CLUSTER_DATANODE,
        HADOOP_CLUSTER_COMPUTENODE,
        KAFKA_CLUSTER,
        ELASTICSEARCH_CLUSTER_DATANODE,
        ELASTICSEARCH_CLUSTER_MASTERNODE,
    ]


class MaintenanceTaskStatusChoices:
    PLANNED = ('PLANNED', 'Planned')
    CANCELED = ('CANCELED', 'Canceled')
    COMPLETED = ('COMPLETED', 'Completed')
    FAILED = ('FAILED', 'Failed')
    REJECTED = ('REJECTED', 'Rejected')

    all = [
        PLANNED,
        CANCELED,
        COMPLETED,
        FAILED,
        REJECTED,
    ]


class MaintenanceWindowDayChoices:
    MON = ('MON', 'Mon')
    TUE = ('TUE', 'Tue')
    WED = ('WED', 'Wed')
    THU = ('THU', 'Thu')
    FRI = ('FRI', 'Fri')
    SAT = ('SAT', 'Sat')
    SUN = ('SUN', 'Sun')

    all = [
        MON,
        TUE,
        WED,
        THU,
        FRI,
        SAT,
        SUN,
    ]


class Geo(django.db.models.Model):
    geo_id = django.db.models.AutoField(primary_key=True)
    name = django.db.models.TextField(unique=True, blank=True, null=True)

    class Meta:
        managed = False
        db_table = '"dbaas"."geo"'

    def __str__(self):
        return str(self.name)


class CloudUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .prefetch_related('folders')\
            .order_by('-clusters_used')


class Cloud(django.db.models.Model, mod_helpers.LinkedMixin):
    cloud_id = django.db.models.BigIntegerField(primary_key=True)
    cloud_ext_id = django.db.models.TextField(unique=True)
    cpu_quota = django.db.models.FloatField()
    memory_quota = django.db.models.BigIntegerField()
    clusters_quota = django.db.models.BigIntegerField()
    cpu_used = django.db.models.FloatField()
    memory_used = django.db.models.BigIntegerField()
    clusters_used = django.db.models.BigIntegerField()
    actual_cloud_rev = django.db.models.BigIntegerField()
    ssd_space_quota = django.db.models.BigIntegerField()
    ssd_space_used = django.db.models.BigIntegerField()
    hdd_space_quota = django.db.models.BigIntegerField()
    hdd_space_used = django.db.models.BigIntegerField()
    gpu_quota = django.db.models.BigIntegerField()
    gpu_used = django.db.models.BigIntegerField()

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = CloudUiListManager()

    class Meta:
        managed = False
        db_table = '"dbaas"."clouds"'

    def __str__(self):
        return str(self.pk)

    def clusters_url(self):
        ids = [str(folder.folder_id) for folder in self.folders.all()]
        if ids:
            str_ids = ','.join(ids)
            return f'/ui/meta/clusters?folder_id={str_ids}'


class CloudRev(django.db.models.Model):
    cloud = django.db.models.ForeignKey(Cloud, django.db.models.DO_NOTHING)
    cloud_rev = django.db.models.BigIntegerField()
    cpu_quota = django.db.models.FloatField()
    memory_quota = django.db.models.BigIntegerField()
    network_quota = django.db.models.BigIntegerField()
    io_quota = django.db.models.BigIntegerField()
    clusters_quota = django.db.models.BigIntegerField()
    cpu_used = django.db.models.FloatField()
    memory_used = django.db.models.BigIntegerField()
    network_used = django.db.models.BigIntegerField()
    io_used = django.db.models.BigIntegerField()
    clusters_used = django.db.models.BigIntegerField()
    x_request_id = django.db.models.TextField(blank=True, null=True)
    commited_at = django.db.models.DateTimeField()
    ssd_space_quota = django.db.models.BigIntegerField()
    ssd_space_used = django.db.models.BigIntegerField()
    hdd_space_quota = django.db.models.BigIntegerField()
    hdd_space_used = django.db.models.BigIntegerField()
    gpu_quota = django.db.models.BigIntegerField()
    gpu_used = django.db.models.BigIntegerField()

    class Meta:
        managed = False
        db_table = '"dbaas"."clouds_revs"'
        unique_together = (('cloud', 'cloud_rev'),)


class FolderUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('cloud')\
            .order_by('-folder_id')


class FolderUiObjManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('cloud')\
            .prefetch_related('cluster_set')\
            .order_by('-folder_id')


class Folder(django.db.models.Model, mod_helpers.LinkedMixin):
    folder_id = django.db.models.BigAutoField(primary_key=True)
    folder_ext_id = django.db.models.TextField(unique=True)
    cloud = django.db.models.ForeignKey(Cloud, django.db.models.DO_NOTHING, related_name='folders')

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = FolderUiListManager()

    class Meta:
        managed = False
        db_table = '"dbaas"."folders"'

    def __str__(self):
        return str(self.folder_ext_id)

    @property
    def not_removed_clusters_url(self):
        statuses = ','.join([s[0] for s in ClusterStatus.not_removed])
        return f'/ui/meta/clusters?folder_id={self.folder_id}&status={statuses}'

    @property
    def not_removed_clusters_count(self):
        statuses = [s[0] for s in ClusterStatus.not_removed]
        return self.cluster_set.filter(status__in=statuses).count()


class ClusterUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .prefetch_related('folder')\
            .order_by('-created_at')


class Cluster(django.db.models.Model, mod_helpers.LinkedMixin):
    cid = django.db.models.CharField(max_length=64, primary_key=True)
    name = django.db.models.TextField()
    type = django.db.models.CharField(max_length=21, choices=ClusterType.all)
    env = django.db.models.CharField(max_length=16, choices=EnvType.all)
    created_at = django.db.models.DateTimeField()
    public_key = django.db.models.BinaryField(blank=True, null=True)
    network_id = django.db.models.TextField()
    folder = django.db.models.ForeignKey(Folder, django.db.models.DO_NOTHING)
    description = django.db.models.TextField(blank=True, null=True, default='')
    status = django.db.models.CharField(max_length=21, choices=ClusterStatus.all)
    actual_rev = django.db.models.BigIntegerField()
    next_rev = django.db.models.BigIntegerField()

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = ClusterUiListManager()

    class Meta:
        managed = False
        db_table = '"dbaas"."clusters"'
        unique_together = (('folder', 'type', 'name'),)

    def __str__(self):
        return str(self.pk)

    @property
    def hosts(self):
        return Host.objects.filter(subcluster_id__in=self.subclusters.all())\
            .select_related('shard')\
            .select_related('geo')

    @property
    def maintenance_window(self):
        return MaintenanceWindowSetting.objects.filter(cid=self.cid).first()

    @property
    def worker_tasks(self):
        return WorkerTask.objects.filter(cid=self.cid)

    @property
    def links(self):
        links = []
        if self.type.endswith('_cluster'):
            type_suffix = self.type[:-len('_cluster')]
        else:
            type_suffix = self.type

        if settings.INSTALLATION.is_porto():
            if settings.INSTALLATION.name == 'prod':
                yc_host = 'yc.yandex-team.ru'
            else:
                yc_host = 'yc-test.yandex-team.ru'
            links.extend([
                {
                    'name': 'Yandex Cloud UI',
                    'url': f'https://{yc_host}/folders/mdb-junk/managed-{type_suffix}/cluster/{self.cid}',
                },
                {
                    'name': 'Solomon',
                    'url': f'https://solomon.yandex-team.ru/?cluster=mdb_{self.cid}&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-{type_suffix}',
                },
                {
                    'name': 'YASM',
                    'url': f'https://yasm.yandex-team.ru/template/panel/dbaas_{type_suffix}_metrics/cid={self.cid}',
                }
            ])
        elif settings.INSTALLATION.is_compute():
            if settings.INSTALLATION.name == 'prod':
                links.extend([
                    {
                        'name': 'Yandex Cloud UI',
                        'url': f'https://console.cloud.yandex.ru/folders/b1g0r9fh49hee3rsc0aa/managed-{type_suffix}/cluster/{self.cid}',
                    },
                    {
                        'name': 'YASM',
                        'url': f'https://yasm.yandex-team.ru/template/panel/dbaas_{type_suffix}_metrics/cid={self.cid}',
                    }
                ])
            elif settings.INSTALLATION.name == 'preprod':
                links.extend([
                    {
                        'name': 'Yandewx Cloud UI',
                        'url': f'https://console-preprod.cloud.yandex.ru/folders/aoe3pfp0comvds8269mr/managed-{type_suffix}/cluster/{self.cid}',
                    },
                ])
        return links

    def get_hosts(self):
        return Host.ui_list\
            .filter(subcid__in=self.subclusters)

    def get_prev_rev(self, rev):
        if rev is None:
            return None

        cr = (
            ClusterRev.objects.filter(cid=self.cid, rev__lte=rev)
            .order_by('-rev')
            .values('rev', 'name', 'network_id', 'folder_id', 'description', 'status')
            .first()
        )
        if cr:
            return cr

    def get_historical_subclusters(self):
        return {scr['subcid'] for scr in SubclusterRev.objects.filter(cid=self.cid).values('subcid')}

    def get_historical_hosts(self):
        return {
            scr['fqdn'] for scr in HostRev.objects.filter(subcluster_id__in=self.get_historical_subclusters()).values('fqdn')
        }


class ClusterRev(django.db.models.Model):
    cid = django.db.models.CharField(max_length=64, primary_key=True)
    rev = django.db.models.BigIntegerField()
    name = django.db.models.TextField()
    network_id = django.db.models.TextField()
    folder_id = django.db.models.BigIntegerField()
    description = django.db.models.TextField(blank=True, null=True)
    status = django.db.models.CharField(max_length=21, choices=ClusterStatus.all)

    class Meta:
        managed = False
        db_table = '"dbaas"."clusters_revs"'
        unique_together = (('cid', 'rev'),)


class ClusterChange(django.db.models.Model):
    cid = django.db.models.CharField(max_length=64, primary_key=True)
    rev = django.db.models.BigIntegerField()
    changes = django.db.models.JSONField()
    committed_at = django.db.models.DateTimeField()
    x_request_id = django.db.models.TextField(blank=True, null=True)
    commit_id = django.db.models.BigIntegerField()

    class Meta:
        managed = False
        db_table = '"dbaas"."clusters_changes"'
        unique_together = (('cid', 'rev'),)


class SubclusterUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('cluster')\
            .order_by('-created_at')


class SubclusterUiObjManager(SubclusterUiListManager):
    def get_queryset(self):
        return super().get_queryset()


class Subcluster(django.db.models.Model, mod_helpers.LinkedMixin):
    subcid = django.db.models.TextField(primary_key=True)
    cluster = django.db.models.ForeignKey(Cluster, django.db.models.DO_NOTHING, db_column='cid', related_name='subclusters')
    name = django.db.models.TextField()
    roles = pg_fields.ArrayField(django.db.models.CharField(max_length=32, choices=RoleTypeChoices.all))
    created_at = django.db.models.DateTimeField()

    objects = django.db.models.Manager()
    ui_obj = SubclusterUiObjManager()
    ui_list = SubclusterUiListManager()

    class Meta:
        managed = False
        db_table = '"dbaas"."subclusters"'
        unique_together = (('cluster', 'name'),)

    def __str__(self):
        return str(self.subcid)

    def get_revs(self):
        return (
            SubclusterRev.objects.filter(subcid=self.subcid)
            .order_by('-rev')
            .values('rev', 'cid', 'name', 'roles', 'created_at')
        )

    def get_shards(self):
        return Shard.objects.filter(subcluster_id=self.subcid)

    def get_hosts(self):
        return Host.ui_list\
            .filter(subcluster_id=self.subcid)

    @staticmethod
    def get_prev_rev(cid, rev):
        if rev is None:
            return None

        cr = (
            SubclusterRev.objects.filter(cid=cid, rev__lte=rev)
            .order_by('-rev')
            .values('rev', 'cid', 'name', 'roles', 'created_at')
            .first()
        )
        if cr:
            return cr


class SubclusterRev(django.db.models.Model):
    subcid = django.db.models.TextField()
    rev = django.db.models.BigIntegerField()
    cid = django.db.models.ForeignKey(Cluster, django.db.models.DO_NOTHING, db_column='cid')
    name = django.db.models.TextField()
    roles = pg_fields.ArrayField(django.db.models.CharField(max_length=32, choices=RoleTypeChoices.all))
    created_at = django.db.models.DateTimeField()

    class Meta:
        managed = False
        db_table = '"dbaas"."subclusters_revs"'
        unique_together = (('subcid', 'rev'),)


class ShardUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('subcluster')\
            .order_by('-created_at')


class ShardUiObjManager(ShardUiListManager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('pillar')\
            .prefetch_related('host_set')


class Shard(django.db.models.Model, mod_helpers.LinkedMixin):
    subcluster = django.db.models.ForeignKey(Subcluster, django.db.models.DO_NOTHING, db_column='subcid')
    shard_id = django.db.models.TextField(primary_key=True)
    name = django.db.models.TextField()
    created_at = django.db.models.DateTimeField()

    objects = django.db.models.Manager()
    ui_obj = ShardUiObjManager()
    ui_list = ShardUiListManager()

    class Meta:
        managed = False
        db_table = '"dbaas"."shards"'
        unique_together = (('subcluster', 'name'),)

    def __str__(self):
        return str(self.shard_id)

    def get_revs(self):
        return (
            ShardRev.objects.filter(shard_id=self.shard_id)
            .order_by('-rev')
            .values('rev', 'subcid', 'name', 'created_at')
        )


class ShardRev(django.db.models.Model):
    subcid = django.db.models.ForeignKey(Subcluster, django.db.models.DO_NOTHING, db_column='subcid')
    shard_id = django.db.models.ForeignKey(Shard, django.db.models.DO_NOTHING, db_column='shard_id')
    rev = django.db.models.BigIntegerField()
    name = django.db.models.TextField()
    created_at = django.db.models.DateTimeField()

    class Meta:
        managed = False
        db_table = '"dbaas"."shards_revs"'
        unique_together = (('shard_id', 'rev'),)


class DiskType(django.db.models.Model):
    disk_type_id = django.db.models.BigIntegerField(primary_key=True)
    disk_type_ext_id = django.db.models.CharField(max_length=256, unique=True)
    quota_type = django.db.models.CharField(max_length=4, choices=[(x, x) for x in ['ssd', 'hdd']])

    class Meta:
        managed = False
        db_table = '"dbaas"."disk_type"'

    def __str__(self):
        return str(self.disk_type_ext_id)


class FlavorType(django.db.models.Model):
    id = django.db.models.AutoField(unique=True, primary_key=True)
    type = django.db.models.TextField()
    generation = django.db.models.IntegerField()

    class Meta:
        managed = False
        db_table = '"dbaas"."flavor_type"'
        unique_together = (('type', 'generation'),)


class FlavorUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()


class Flavor(django.db.models.Model, mod_helpers.LinkedMixin):
    id = django.db.models.UUIDField(primary_key=True)
    cpu_guarantee = django.db.models.FloatField()
    cpu_limit = django.db.models.FloatField()
    memory_guarantee = django.db.models.BigIntegerField()
    memory_limit = django.db.models.BigIntegerField()
    network_guarantee = django.db.models.BigIntegerField()
    network_limit = django.db.models.BigIntegerField()
    io_limit = django.db.models.BigIntegerField()
    name = django.db.models.TextField(unique=True)
    visible = django.db.models.BooleanField()
    vtype = django.db.models.CharField(max_length=7, choices=[(x, x) for x in ['porto', 'compute']])
    platform_id = django.db.models.TextField()
    type = django.db.models.CharField(max_length=32)
    generation = django.db.models.IntegerField()
    gpu_limit = django.db.models.IntegerField()
    io_cores_limit = django.db.models.IntegerField()

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = FlavorUiListManager()

    class Meta:
        managed = False
        db_table = '"dbaas"."flavors"'

    def __str__(self):
        return str(self.name)


class HostUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .prefetch_related('flavor')\
            .prefetch_related('geo')\
            .prefetch_related('subcluster')\
            .prefetch_related('subcluster__cluster')\
            .order_by('-created_at')


class HostUiObjManager(HostUiListManager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('shard')\
            .select_related('disk_type')


class Host(django.db.models.Model, mod_helpers.LinkedMixin):
    fqdn = django.db.models.TextField(primary_key=True)
    subcluster = django.db.models.ForeignKey(Subcluster, django.db.models.DO_NOTHING, db_column='subcid')
    shard = django.db.models.ForeignKey(Shard, django.db.models.DO_NOTHING, blank=True, null=True)
    flavor = django.db.models.ForeignKey(Flavor, django.db.models.DO_NOTHING, db_column='flavor')
    space_limit = django.db.models.BigIntegerField()
    vtype_id = django.db.models.TextField(blank=True, null=True)
    geo = django.db.models.ForeignKey(Geo, django.db.models.DO_NOTHING)
    disk_type = django.db.models.ForeignKey(DiskType, django.db.models.DO_NOTHING)
    subnet_id = django.db.models.TextField()
    assign_public_ip = django.db.models.BooleanField()
    created_at = django.db.models.DateTimeField()

    objects = django.db.models.Manager()
    ui_obj = HostUiObjManager()
    ui_list = HostUiListManager()

    class Meta:
        managed = False
        db_table = '"dbaas"."hosts"'

    def __str__(self):
        return str(self.fqdn)

    def get_revs(self):
        return HostRev.objects.filter(fqdn=self.fqdn).order_by('-rev')

    @staticmethod
    def get_prev_rev(fqdn, rev):
        if rev is None:
            return None

        cr = (
            HostRev.objects.filter(fqdn=fqdn, rev__lte=rev)
            .order_by('-rev')
            .values(
                'rev',
                'flavor',
                'space_limit',
                'vtype_id',
                'geo',
                'disk_type',
                'subnet_id',
                'assign_public_ip',
                'created_at',
            )
            .first()
        )
        if cr:
            return cr

    @property
    def links(self):
        links = []
        if settings.INSTALLATION.is_porto():
            links.append({
                'name': 'Dom0',
                'url': f'https://solomon.yandex-team.ru/?cluster=internal-mdb_dom0&dc=by_cid_container&host=by_cid_container&project=internal-mdb&service=dom0&dashboard=internal-mdb-porto-instance&l.container={self.fqdn}&b=1h&e='  # noqa: E501
            })
        elif settings.INSTALLATION.is_compute():
            if settings.INSTALLATION.name == 'prod':
                links.extend([
                    {
                        'name': 'Dom0',
                        'url': f'https://solomon.cloud.yandex-team.ru/?project=yandexcloud&service=yandexcloud_dbaas&dashboard=cloud-mdb-instance-system&cluster=mdb_{self.subcluster.cluster_id}&b=1d&e=&l.host={self.fqdn}',  # noqa: E501
                    },
                    {
                        'name': 'Compute instance',
                        'url': f'https://solomon.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_compute&service=compute&dashboard=compute-instance-health&l.instance_id={self.vtype_id}',  # noqa: E501
                    }
                ])

        return links


class HostRev(django.db.models.Model):
    subcluster = django.db.models.ForeignKey(Subcluster, django.db.models.DO_NOTHING, db_column='subcid')
    shard = django.db.models.ForeignKey(Shard, django.db.models.DO_NOTHING, blank=True, null=True)
    flavor = django.db.models.ForeignKey(Flavor, django.db.models.DO_NOTHING, db_column='flavor')
    space_limit = django.db.models.BigIntegerField()
    fqdn = django.db.models.TextField(primary_key=True)  # fake primary key for ORM works
    rev = django.db.models.BigIntegerField()
    vtype_id = django.db.models.TextField(blank=True, null=True)
    geo = django.db.models.ForeignKey(Geo, django.db.models.DO_NOTHING)
    disk_type = django.db.models.ForeignKey(DiskType, django.db.models.DO_NOTHING)
    subnet_id = django.db.models.TextField()
    assign_public_ip = django.db.models.BooleanField()
    created_at = django.db.models.DateTimeField()

    class Meta:
        managed = False
        db_table = '"dbaas"."hosts_revs"'
        unique_together = (('fqdn', 'rev'),)


class DefaultPillar(django.db.models.Model):
    id = django.db.models.BigIntegerField(primary_key=True)
    value = django.db.models.JSONField()

    class Meta:
        managed = False
        db_table = '"dbaas"."default_pillar"'


class ClusterTypePillar(django.db.models.Model):
    type = django.db.models.CharField(max_length=21, choices=ClusterType.all)
    value = django.db.models.JSONField()

    class Meta:
        managed = False
        db_table = '"dbaas"."cluster_type_pillar"'


class RolePillar(django.db.models.Model):
    type = django.db.models.CharField(max_length=32, choices=ClusterType.all)
    role = django.db.models.CharField(max_length=32, choices=RoleTypeChoices.all)
    value = django.db.models.JSONField()

    class Meta:
        managed = False
        db_table = '"dbaas"."role_pillar"'
        unique_together = (('type', 'role'),)


class Pillar(django.db.models.Model):
    cid = django.db.models.OneToOneField(Cluster, django.db.models.DO_NOTHING, db_column='cid', primary_key=True)
    subcid = django.db.models.OneToOneField(Subcluster, django.db.models.DO_NOTHING, db_column='subcid', blank=True, null=True)
    shard = django.db.models.OneToOneField(Shard, django.db.models.DO_NOTHING, db_column='shard_id', blank=True, null=True)
    fqdn = django.db.models.OneToOneField(Host, django.db.models.DO_NOTHING, db_column='fqdn', blank=True, null=True)
    value = django.db.models.JSONField()

    class Meta:
        managed = False
        db_table = '"dbaas"."pillar"'
        unique_together = (('cid', 'subcid', 'shard', 'fqdn'),)

    @staticmethod
    def get_value(**kwargs):
        pillars = Pillar.objects.filter(**kwargs).values('value')
        if pillars:
            return pillars[0]['value']


class PillarRev(django.db.models.Model):
    rev = django.db.models.BigIntegerField()
    cid = django.db.models.CharField(max_length=64, primary_key=True)
    subcid = django.db.models.ForeignKey(Subcluster, django.db.models.DO_NOTHING, db_column='subcid', blank=True, null=True)
    shard = django.db.models.ForeignKey(Shard, django.db.models.DO_NOTHING, blank=True, null=True)
    fqdn = django.db.models.ForeignKey(Host, django.db.models.DO_NOTHING, db_column='fqdn', blank=True, null=True)
    value = django.db.models.JSONField()

    class Meta:
        managed = False
        db_table = '"dbaas"."pillar_revs"'
        unique_together = (
            ('cid', 'rev'),
            ('fqdn', 'rev'),
            ('shard', 'rev'),
            ('subcid', 'rev'),
        )

    @staticmethod
    def get_prev_rev(rev, **kwargs):
        if rev is None:
            return None

        pr = PillarRev.objects.filter(rev__lte=rev, **kwargs).order_by('-rev').values('rev', 'value').first()
        if pr:
            return pr

    @staticmethod
    def get_changes(pillar_revs, with_revs=True):
        changes = []
        for i in range(1, len(pillar_revs)):
            if with_revs:
                changes.append(
                    (
                        pillar_revs[i]['rev'],
                        list(
                            dictdiffer.diff(
                                pillar_revs[i - 1]['value'],
                                pillar_revs[i]['value'],
                            )
                        )
                    )
                )
            else:
                old = pillar_revs[i - 1].get('value', {}) if pillar_revs[i - 1] else {}
                new = pillar_revs[i].get('value', {}) if pillar_revs[i] else {}
                changes.append((list(dictdiffer.diff(old, new))))
        return changes


class WorkerTaskUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('cluster')\
            .order_by('-create_ts')


class WorkerTaskUiObjManager(WorkerTaskUiListManager):
    pass


class WorkerTaskAbstract(django.db.models.Model, mod_helpers.LinkedMixin):
    acquire_rev = django.db.models.BigIntegerField(blank=True, null=True)
    changes = django.db.models.JSONField()
    cluster = django.db.models.ForeignKey(Cluster, django.db.models.DO_NOTHING, db_column='cid')
    comment = django.db.models.TextField(blank=True, null=True)
    config_id = django.db.models.TextField(blank=True, null=True)
    context = django.db.models.JSONField(blank=True, null=True)
    created_by = django.db.models.TextField(blank=True, null=True)
    create_rev = django.db.models.BigIntegerField()
    create_ts = django.db.models.DateTimeField()
    delayed_until = django.db.models.DateTimeField(blank=True, null=True)
    end_ts = django.db.models.DateTimeField(blank=True, null=True)
    errors = django.db.models.JSONField(blank=True, null=True)
    finish_rev = django.db.models.BigIntegerField(blank=True, null=True)
    folder = django.db.models.ForeignKey(Folder, django.db.models.DO_NOTHING)
    hidden = django.db.models.BooleanField()
    metadata = django.db.models.JSONField()
    operation_type = django.db.models.TextField()
    required_task = django.db.models.ForeignKey('self', django.db.models.DO_NOTHING, blank=True, null=True)
    restart_count = django.db.models.BigIntegerField(blank=True, default=0)
    failed_acquire_count = django.db.models.BigIntegerField(blank=True, default=0)
    result = django.db.models.BooleanField(null=True)
    start_ts = django.db.models.DateTimeField(blank=True, null=True)
    target_rev = django.db.models.BigIntegerField(blank=True, null=True)
    task_args = django.db.models.JSONField()
    task_id = django.db.models.TextField(primary_key=True)
    task_type = django.db.models.TextField()
    timeout = django.db.models.DurationField()
    tracing = django.db.models.TextField(blank=True, null=True)
    unmanaged = django.db.models.BooleanField()
    version = django.db.models.IntegerField()
    worker_id = django.db.models.TextField(blank=True, null=True)
    notes = django.db.models.TextField(blank=True, null=True)
    cancelled = django.db.models.BooleanField()

    objects = django.db.models.Manager()
    ui_obj = WorkerTaskUiObjManager()
    ui_list = WorkerTaskUiListManager()

    class Meta:
        abstract = True

    @classmethod
    def get_failed_tasks(self):
        query = """SELECT task_id FROM
            (
                SELECT cid, task_id, status as cluster_status, finish_rev
                    FROM dbaas.worker_queue JOIN dbaas.clusters using (cid)
                    WHERE result=false AND status IN ('CREATE-ERROR', 'MODIFY-ERROR', 'STOP-ERROR', 'START-ERROR', 'DELETE-ERROR', 'PURGE-ERROR', 'METADATA-DELETE-ERROR')
            ) tasks
            JOIN dbaas.clusters_revs cr ON (tasks.cid = cr.cid AND tasks.finish_rev = cr.rev)
                WHERE dbaas.error_cluster_status(status)"""
        return WorkerTask.objects.raw(query)

    @property
    def shipments_count(self):
        try:
            return len(self.get_shipments_ids())
        except Exception:
            return -1

    @property
    def comment_is_traceback(self):
        if self.comment and self.comment.startswith('Traceback (most recent call last):'):
            return True
        else:
            return True

    def cancel(self, username, comment):
        query_cancel = """
            SELECT code.cancel_task(%(task_id)s)
            FROM dbaas.worker_queue
            WHERE task_id=%(task_id)s
        """
        query_notes = """
            UPDATE dbaas.worker_queue
            SET notes=%(notes)s
            WHERE task_id=%(task_id)s
        """
        with django.db.transaction.atomic(using='meta_db'):
            with django.db.connections['meta_db'].cursor() as cursor:
                cursor.execute(
                    query_cancel,
                    {'task_id': self.pk}
                )
                cursor.execute(
                    query_notes,
                    {'task_id': self.pk, 'notes': f'Cancelled by {username} via UI: {comment}'},
                )

    def restart(self, username, comment):
        query = """
            SELECT code.restart_task(
                i_task_id => %(task_id)s,
                i_notes => %(notes)s
            )
        """
        with django.db.connections['meta_db'].cursor() as cursor:
            cursor.execute(
                query,
                {'task_id': self.pk, 'notes': f'Restarted by {username} via UI: {comment}'},
            )

    def reject(self, username, comment):
        query = """
            SELECT code.reject_failed_task(%(task_id)s, %(username)s, %(comment)s)
            FROM dbaas.worker_queue
            WHERE task_id=%(task_id)s AND result=false
        """
        with django.db.connections['meta_db'].cursor() as cursor:
            cursor.execute(
                query,
                {
                    'task_id': self.pk,
                    'username': username,
                    'comment': f'Rejected by {username} via UI: {comment}',
                },
            )

    def finish(self, username, comment):
        query = """
            SELECT code.finish_task(worker_id, %(task_id)s, true, '{}', %(comment)s)
            FROM dbaas.worker_queue
            WHERE task_id=%(task_id)s AND result=false
        """
        with django.db.connections['meta_db'].cursor() as cursor:
            cursor.execute(
                query,
                {
                    'task_id': self.pk,
                    'comment': f'Finished by {username} via UI: {comment}',
                },
            )

    def check_cluster_status_for_restart(self):
        query = """
            SELECT 1 FROM code.cluster_status_acquire_transitions()
            WHERE from_status = %(cluster_status)s
            AND action = code.task_type_action(%(task_type)s)
        """
        with django.db.connections['meta_db'].cursor() as cursor:
            cursor.execute(query, {'cluster_status': self.cluster.status, 'task_type': self.task_type})
            results = cursor.fetchone()

        if not results:
            return False
        else:
            return True

    def check_reject_or_finish(self):
        if self.result is not False:
            return False, f'invalid task result for reject: {self.result}'
        elif self.cluster.status not in [s[0] for s in ClusterStatus.error]:
            return False, f'invalid cluster status: {self.cluster.status}'
        else:
            return True, None

    def action_ability(self, action, request=None):
        if action == WorkerTaskAction.CANCEL[0]:
            if self.result is not None:
                return False, 'task result is not None'
            elif self.worker_id is None:
                return False, 'task worker_id is None'
            elif self.unmanaged:
                return False, 'task is unmanaged'
            elif self.cancelled:
                return False, 'task is cancelled'
            else:
                return True, None
        elif action == WorkerTaskAction.RESTART[0]:
            restart_max_count = settings.CONFIG.get('restart_tasks_max_count', 2)
            if self.result or self.result is None:
                return False, f'invalid task result: {self.result}'
            elif restart_max_count > 0 and self.restart_count > restart_max_count:
                return False, f'too many task restarts: {self.restart}'
            elif self.task_type not in settings.CONFIG.restart_tasks_white_list:
                return False, f'unacceptable task type: {self.task_type}'
            elif not self.check_cluster_status_for_restart():
                return False, 'invalid cluster status'
            else:
                return True, None
        elif action == WorkerTaskAction.REJECT[0] or action == WorkerTaskAction.FINISH[0]:
            return self.check_reject_or_finish()
        else:
            return False, 'unknown action: {action}'

    def parsed_changes(self):
        changes = []
        if self.changes:
            for change in self.changes:
                timestamp = change['timestamp']
                for key, value in change.items():
                    if key != 'timestamp':
                        changes.append({
                            'timestamp': timestamp,
                            'subject': key,
                            'info': value,
                        })
                        break
        return changes

    def pretty_args(self):
        return json.dumps(self.task_args, sort_keys=True, indent=4)

    def get_shipments_jobs(self):
        shipment_ids = self.get_shipments_ids()
        if not shipment_ids:
            return []
        else:
            return deploy_models.Job.objects\
                .select_related(
                    'command__shipment_command__shipment',
                    'command__shipment_command',
                    'command__minion',
                    'command',
                )\
                .filter(
                    command__shipment_command__shipment_id__in=shipment_ids,
                )

    def get_shipments_ids(self):
        shipment_ids = set()
        if self.comment:
            for row in self.comment.split('\n'):
                if 'deployv2.components' in row:
                    substr = row[row.find('[') + 1 : row.find(']')]
                    shipment_ids.update(re.findall(r'[. ](\d+)[,()]', substr))
        if self.changes:
            for ch in self.changes:
                for row in ch.keys():
                    if row.startswith('deployv2'):
                        shipment_ids.add(row.split('.')[-1])
        return shipment_ids


class WorkerTask(WorkerTaskAbstract):
    class Meta:
        managed = False
        db_table = '"dbaas"."worker_queue"'
        unique_together = (('create_ts', 'task_id'),)

    def __str__(self):
        return str(self.task_id)


class WorkerTaskRestartHistory(WorkerTaskAbstract):
    class Meta:
        managed = False
        db_table = '"dbaas"."worker_queue_restart_history"'
        unique_together = (('task_id', 'restart_count'),)

    def __str__(self):
        return str(f'Task {self.task_id} restart attempt {self.restart_count}')


class DefaultFeatureFlag(django.db.models.Model):
    flag_name = django.db.models.TextField(primary_key=True)

    class Meta:
        managed = False
        db_table = '"dbaas"."default_feature_flags"'


class CloudFeatureFlag(django.db.models.Model):
    cloud = django.db.models.OneToOneField(Cloud, django.db.models.DO_NOTHING)
    flag_name = django.db.models.TextField()

    class Meta:
        managed = False
        db_table = '"dbaas"."cloud_feature_flags"'
        unique_together = (('cloud', 'flag_name'),)


class ValidResourceUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .prefetch_related('flavor')\
            .prefetch_related('geo')\
            .prefetch_related('disk_type')\
            .order_by('-id')


class ValidResourceUiObjManager(ValidResourceUiListManager):
    pass


class ValidResource(django.db.models.Model, mod_helpers.LinkedMixin):
    id = django.db.models.IntegerField(primary_key=True)
    cluster_type = django.db.models.CharField(max_length=32, choices=ClusterType.all)
    role = django.db.models.CharField(max_length=32, choices=RoleTypeChoices.all)
    flavor = django.db.models.ForeignKey(Flavor, django.db.models.DO_NOTHING, db_column='flavor')
    disk_type = django.db.models.ForeignKey(DiskType, django.db.models.DO_NOTHING)
    geo = django.db.models.ForeignKey(Geo, django.db.models.DO_NOTHING)
    disk_size_range = django.db.models.CharField(max_length=64, blank=True, null=True)
    disk_sizes = pg_fields.ArrayField(django.db.models.BigIntegerField())
    min_hosts = django.db.models.IntegerField()
    max_hosts = django.db.models.IntegerField()
    feature_flag = django.db.models.TextField(blank=True, null=True)

    objects = django.db.models.Manager()
    ui_obj = ValidResourceUiObjManager()
    ui_list = ValidResourceUiListManager()

    class Meta:
        managed = False
        db_table = '"dbaas"."valid_resources"'
        unique_together = (('cluster_type', 'role', 'flavor', 'disk_type', 'geo'),)

    def __str__(self):
        return str(self.pk)


class VersionUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('cluster')\
            .select_related('subcluster')\
            .select_related('shard')\
            .order_by('package_version')


class Version(django.db.models.Model):
    cluster = django.db.models.ForeignKey(Cluster, django.db.models.DO_NOTHING, db_column='cid', related_name='versions', blank=True, null=True)
    subcluster = django.db.models.ForeignKey(Subcluster, django.db.models.DO_NOTHING, db_column='subcid', blank=True, null=True)
    shard = django.db.models.ForeignKey(Shard, django.db.models.DO_NOTHING, blank=True, null=True)
    component = django.db.models.TextField()
    major_version = django.db.models.TextField()
    minor_version = django.db.models.TextField()
    package_version = django.db.models.TextField()
    edition = django.db.models.TextField(primary_key=True)  # fake primary key for ORM works

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = VersionUiListManager()

    class Meta:
        managed = False
        db_table = '"dbaas"."versions"'
        unique_together = (
            ('cluster', 'component'),
            ('shard', 'component'),
            ('subcluster', 'component'),
        )


class DefaultVersionUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .order_by('package_version')


class DefaultVersion(django.db.models.Model):
    type = django.db.models.CharField(max_length=21, choices=ClusterType.all)
    component = django.db.models.TextField()
    major_version = django.db.models.TextField()
    minor_version = django.db.models.TextField()
    package_version = django.db.models.TextField(primary_key=True)
    env = django.db.models.CharField(max_length=21, choices=EnvType.all)
    is_deprecated = django.db.models.BooleanField()
    is_default = django.db.models.BooleanField()
    updatable_to = pg_fields.ArrayField(django.db.models.CharField(max_length=16))
    name = django.db.models.TextField()
    edition = django.db.models.TextField()

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = DefaultVersionUiListManager()

    class Meta:
        managed = False
        db_table = '"dbaas"."default_versions"'
        unique_together = (('type', 'component', 'name', 'env'), ('type', 'component', 'env'),)


class MaintenanceTaskUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('cluster')\
            .order_by('-create_ts')


class MaintenanceTask(django.db.models.Model, mod_helpers.LinkedMixin):
    cluster = django.db.models.OneToOneField(
        Cluster,
        django.db.models.DO_NOTHING,
        db_column='cid',
        primary_key=True,
    )
    config_id = django.db.models.TextField()
    task_id = django.db.models.TextField()
    create_ts = django.db.models.DateTimeField()
    plan_ts = django.db.models.DateTimeField(blank=True, null=True)
    status = django.db.models.CharField(max_length=16, choices=MaintenanceTaskStatusChoices.all)
    info = django.db.models.TextField(blank=True, null=True)

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = MaintenanceTaskUiListManager()

    class Meta:
        managed = False
        db_table = '"dbaas"."maintenance_tasks"'
        unique_together = (('cluster', 'config_id'),)

    def __str__(self):
        return str(self.task_id)

    @property
    def self_primary_key(self):
        return self.task_id


class MaintenanceWindowSetting(django.db.models.Model):
    cid = django.db.models.OneToOneField(Cluster, django.db.models.DO_NOTHING, db_column='cid', primary_key=True)
    day = django.db.models.CharField(max_length=4, choices=MaintenanceWindowDayChoices.all)
    hour = django.db.models.IntegerField()

    class Meta:
        managed = False
        db_table = '"dbaas"."maintenance_window_settings"'


class BackupUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('cluster')\
            .order_by('-created_at')


class BackupUiObjManager(BackupUiListManager):
    pass


class Backup(django.db.models.Model, mod_helpers.LinkedMixin):
    backup_id = django.db.models.TextField(primary_key=True)
    cluster = django.db.models.ForeignKey(Cluster, django.db.models.DO_NOTHING, db_column='cid')
    subcluster = django.db.models.ForeignKey(Subcluster, django.db.models.DO_NOTHING, db_column='subcid', blank=True, null=True)
    shard = django.db.models.ForeignKey(Shard, django.db.models.DO_NOTHING, blank=True, null=True)
    status = django.db.models.CharField(max_length=32, choices=BackupStatus.all)
    scheduled_date = django.db.models.DateField(blank=True, null=True)
    created_at = django.db.models.DateTimeField()
    delayed_until = django.db.models.DateTimeField()
    started_at = django.db.models.DateTimeField(blank=True, null=True)
    finished_at = django.db.models.DateTimeField(blank=True, null=True)
    updated_at = django.db.models.DateTimeField(blank=True, null=True)
    shipment_id = django.db.models.TextField(blank=True, null=True)
    metadata = django.db.models.JSONField()
    errors = django.db.models.JSONField()
    initiator = django.db.models.CharField(max_length=32, choices=BackupInitiator.all)
    method = django.db.models.CharField(max_length=32, choices=BackupMethod.all)

    objects = django.db.models.Manager()
    ui_obj = BackupUiObjManager()
    ui_list = BackupUiListManager()

    class Meta:
        managed = False
        db_table = '"dbaas"."backups"'

    def __str__(self):
        return self.backup_id

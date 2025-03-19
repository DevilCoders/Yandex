import re

from dictdiffer import diff
from django.contrib.postgres.fields import ArrayField, JSONField
from django.db import models

from cloud.mdb.ui.internal.common import LinkedMixin

CLUSTER_TYPES = [
    'postgresql_cluster',
    'pgaas-proxy',
    'zookeeper',
    'clickhouse_cluster',
    'mongodb_cluster',
    'redis_cluster',
    'mysql_cluster',
    'sqlserver_cluster',
    'hadoop_cluster',
    'kafka_cluster',
    'elasticsearch_cluster',
]

ENV_TYPES = [
    'dev',
    'load',
    'qa',
    'prod',
    'compute-prod',
]

CLUSTER_STATUSES = [
    'CREATING',
    'CREATE-ERROR',
    'RUNNING',
    'MODIFYING',
    'MODIFY-ERROR',
    'STOPPING',
    'STOP-ERROR',
    'STOPPED',
    'STARTING',
    'START-ERROR',
    'DELETING',
    'DELETE-ERROR',
    'DELETED',
    'PURGING',
    'PURGE-ERROR',
    'PURGED',
    'METADATA-DELETING',
    'METADATA-DELETE-ERROR',
    'METADATA-DELETED',
]

ROLE_TYPES = [
    'postgresql_cluster',
    'clickhouse_cluster',
    'pgaas-proxy',
    'mongodb_cluster',
    'zk',
    'redis_cluster',
    'mysql_cluster',
    'sqlserver_cluster',
    'mongodb_cluster.mongod',
    'mongodb_cluster.mongocfg',
    'mongodb_cluster.mongos',
    'mongodb_cluster.mongoinfra',
    'hadoop_cluster.masternode',
    'hadoop_cluster.datanode',
    'hadoop_cluster.computenode',
    'kafka_cluster',
    'elasticsearch_cluster.datanode',
    'elasticsearch_cluster.masternode',
]

MAINTENANCE_TASK_STATUSES = [
    'PLANNED',
    'CANCELED',
    'COMPLETED',
    'FAILED',
    'REJECTED',
]

MAINTENANCE_WINDOW_DAYS = ['MON', 'TUE', 'WED', 'THU', 'FRI', 'SAT', 'SUN']


class Geo(models.Model):
    geo_id = models.AutoField(primary_key=True)
    name = models.TextField(unique=True, blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'geo'

    def __str__(self):
        return self.name


class Cloud(models.Model, LinkedMixin):
    cloud_id = models.BigIntegerField(primary_key=True)
    cloud_ext_id = models.TextField(unique=True)
    cpu_quota = models.FloatField()
    memory_quota = models.BigIntegerField()
    clusters_quota = models.BigIntegerField()
    cpu_used = models.FloatField()
    memory_used = models.BigIntegerField()
    clusters_used = models.BigIntegerField()
    actual_cloud_rev = models.BigIntegerField()
    ssd_space_quota = models.BigIntegerField()
    ssd_space_used = models.BigIntegerField()
    hdd_space_quota = models.BigIntegerField()
    hdd_space_used = models.BigIntegerField()
    gpu_quota = models.BigIntegerField()
    gpu_used = models.BigIntegerField()

    class Meta:
        managed = False
        db_table = 'clouds'

    def __str__(self):
        return self.cloud_ext_id


class CloudRev(models.Model):
    cloud = models.ForeignKey(Cloud, models.DO_NOTHING)
    cloud_rev = models.BigIntegerField()
    cpu_quota = models.FloatField()
    memory_quota = models.BigIntegerField()
    network_quota = models.BigIntegerField()
    io_quota = models.BigIntegerField()
    clusters_quota = models.BigIntegerField()
    cpu_used = models.FloatField()
    memory_used = models.BigIntegerField()
    network_used = models.BigIntegerField()
    io_used = models.BigIntegerField()
    clusters_used = models.BigIntegerField()
    x_request_id = models.TextField(blank=True, null=True)
    commited_at = models.DateTimeField()
    ssd_space_quota = models.BigIntegerField()
    ssd_space_used = models.BigIntegerField()
    hdd_space_quota = models.BigIntegerField()
    hdd_space_used = models.BigIntegerField()
    gpu_quota = models.BigIntegerField()
    gpu_used = models.BigIntegerField()

    class Meta:
        managed = False
        db_table = 'clouds_revs'
        unique_together = (('cloud', 'cloud_rev'),)


class Folder(models.Model, LinkedMixin):
    folder_id = models.BigAutoField(primary_key=True)
    folder_ext_id = models.TextField(unique=True)
    cloud = models.ForeignKey(Cloud, models.DO_NOTHING, related_name='folders')

    class Meta:
        managed = False
        db_table = 'folders'

    def __str__(self):
        return self.folder_ext_id


class Cluster(models.Model, LinkedMixin):
    cid = models.CharField(max_length=64, primary_key=True)
    name = models.TextField()
    type = models.CharField(max_length=16, choices=[(x, x) for x in CLUSTER_TYPES])
    env = models.CharField(max_length=16, choices=[(x, x) for x in ENV_TYPES])
    created_at = models.DateTimeField()
    public_key = models.BinaryField(blank=True, null=True)
    network_id = models.TextField()
    folder = models.ForeignKey('Folder', models.DO_NOTHING)
    description = models.TextField(blank=True, null=True, default='')
    status = models.CharField(max_length=16, choices=[(x, x) for x in CLUSTER_STATUSES])
    actual_rev = models.BigIntegerField()
    next_rev = models.BigIntegerField()

    class Meta:
        managed = False
        db_table = 'clusters'
        unique_together = (('folder', 'type', 'name'),)

    def __str__(self):
        return self.cid

    @property
    def hosts(self):
        return [h['fqdn'] for h in Host.objects.filter(subcid__in=self.subclusters.all()).values('fqdn')]

    def get_hosts(self):
        return Host.objects.filter(subcid__in=self.subclusters.all())

    def get_revs(self):
        return (
            ClusterRev.objects.filter(cid=self.cid)
            .order_by('-rev')
            .values('rev', 'name', 'network_id', 'folder_id', 'description', 'status')
        )

    def get_prev_rev(self, rev):
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
            scr['fqdn'] for scr in HostRev.objects.filter(subcid__in=self.get_historical_subclusters()).values('fqdn')
        }

    def get_versions(self):
        return Version.objects.filter(cid=self.cid).values(
            'component', 'major_version', 'minor_version', 'package_version', 'edition'
        )

    def get_maint_window(self):
        return MaintenanceWindowSetting.objects.filter(cid=self.cid).values('day', 'hour').first()


class ClusterRev(models.Model):
    cid = models.ForeignKey(Cluster, models.DO_NOTHING, db_column='cid')
    rev = models.BigIntegerField()
    name = models.TextField()
    network_id = models.TextField()
    folder_id = models.BigIntegerField()
    description = models.TextField(blank=True, null=True)
    status = models.CharField(max_length=16, choices=[(x, x) for x in CLUSTER_STATUSES])

    class Meta:
        managed = False
        db_table = 'clusters_revs'
        unique_together = (('cid', 'rev'),)


class ClusterChange(models.Model):
    cid = models.OneToOneField(ClusterRev, models.DO_NOTHING, db_column='cid')
    rev = models.BigIntegerField()
    changes = JSONField()
    committed_at = models.DateTimeField()
    x_request_id = models.TextField(blank=True, null=True)
    commit_id = models.BigIntegerField()

    class Meta:
        managed = False
        db_table = 'clusters_changes'
        unique_together = (('cid', 'rev'),)


class Subcluster(models.Model, LinkedMixin):
    subcid = models.TextField(primary_key=True)
    cid = models.ForeignKey(Cluster, models.DO_NOTHING, db_column='cid', related_name='subclusters')
    name = models.TextField()
    roles = ArrayField(models.CharField(max_length=16, choices=[(x, x) for x in ROLE_TYPES]))
    created_at = models.DateTimeField()

    class Meta:
        managed = False
        db_table = 'subclusters'
        unique_together = (('cid', 'name'),)

    def __str__(self):
        return self.subcid

    def get_revs(self):
        return (
            SubclusterRev.objects.filter(subcid=self.subcid)
            .order_by('-rev')
            .values('rev', 'cid', 'name', 'roles', 'created_at')
        )

    @staticmethod
    def get_prev_rev(cid, rev):
        cr = (
            SubclusterRev.objects.filter(cid=cid, rev__lte=rev)
            .order_by('-rev')
            .values('rev', 'cid', 'name', 'roles', 'created_at')
            .first()
        )
        if cr:
            return cr


class SubclusterRev(models.Model):
    subcid = models.TextField()
    rev = models.BigIntegerField()
    cid = models.ForeignKey(Cluster, models.DO_NOTHING, db_column='cid')
    name = models.TextField()
    roles = ArrayField(models.CharField(max_length=16, choices=[(x, x) for x in ROLE_TYPES]))
    created_at = models.DateTimeField()

    class Meta:
        managed = False
        db_table = 'subclusters_revs'
        unique_together = (('subcid', 'rev'),)


class Shard(models.Model, LinkedMixin):
    subcid = models.ForeignKey(Subcluster, models.DO_NOTHING, db_column='subcid')
    shard_id = models.TextField(primary_key=True)
    name = models.TextField()
    created_at = models.DateTimeField()

    class Meta:
        managed = False
        db_table = 'shards'
        unique_together = (('subcid', 'name'),)

    def __str__(self):
        return self.shard_id

    def get_revs(self):
        return (
            ShardRev.objects.filter(shard_id=self.shard_id)
            .order_by('-rev')
            .values('rev', 'subcid', 'name', 'created_at')
        )


class ShardRev(models.Model):
    subcid = models.ForeignKey(Subcluster, models.DO_NOTHING, db_column='subcid')
    shard_id = models.ForeignKey(Shard, models.DO_NOTHING, db_column='shard_id')
    rev = models.BigIntegerField()
    name = models.TextField()
    created_at = models.DateTimeField()

    class Meta:
        managed = False
        db_table = 'shards_revs'
        unique_together = (('shard_id', 'rev'),)


class DiskType(models.Model):
    disk_type_id = models.BigIntegerField(primary_key=True)
    disk_type_ext_id = models.CharField(max_length=256, unique=True)
    quota_type = models.CharField(max_length=4, choices=[(x, x) for x in ['ssd', 'hdd']])

    class Meta:
        managed = False
        db_table = 'disk_type'

    def __str__(self):
        return self.disk_type_ext_id


class FlavorType(models.Model):
    id = models.AutoField(unique=True, primary_key=True)
    type = models.TextField()
    generation = models.IntegerField()

    class Meta:
        managed = False
        db_table = 'flavor_type'
        unique_together = (('type', 'generation'),)


class Flavor(models.Model):
    id = models.UUIDField(primary_key=True)
    cpu_guarantee = models.FloatField()
    cpu_limit = models.FloatField()
    memory_guarantee = models.BigIntegerField()
    memory_limit = models.BigIntegerField()
    network_guarantee = models.BigIntegerField()
    network_limit = models.BigIntegerField()
    io_limit = models.BigIntegerField()
    name = models.TextField(unique=True)
    visible = models.BooleanField()
    vtype = models.CharField(max_length=4, choices=[(x, x) for x in ['porto', 'compute']])
    platform_id = models.TextField()
    type = models.CharField(max_length=32)
    generation = models.IntegerField()
    gpu_limit = models.IntegerField()
    io_cores_limit = models.IntegerField()

    class Meta:
        managed = False
        db_table = 'flavors'

    def __str__(self):
        return self.name


class Host(models.Model, LinkedMixin):
    subcid = models.ForeignKey(Subcluster, models.DO_NOTHING, db_column='subcid')
    shard = models.ForeignKey(Shard, models.DO_NOTHING, blank=True, null=True)
    flavor = models.ForeignKey(Flavor, models.DO_NOTHING, db_column='flavor')
    space_limit = models.BigIntegerField()
    fqdn = models.TextField(primary_key=True)
    vtype_id = models.TextField(blank=True, null=True)
    geo = models.ForeignKey(Geo, models.DO_NOTHING)
    disk_type = models.ForeignKey(DiskType, models.DO_NOTHING)
    subnet_id = models.TextField()
    assign_public_ip = models.BooleanField()
    created_at = models.DateTimeField()

    class Meta:
        managed = False
        db_table = 'hosts'

    def __str__(self):
        return self.fqdn

    def get_revs(self):
        return (
            HostRev.objects.filter(fqdn=self.fqdn)
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
        )

    @staticmethod
    def get_prev_rev(fqdn, rev):
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


class HostRev(models.Model):
    subcid = models.ForeignKey(Subcluster, models.DO_NOTHING, db_column='subcid')
    shard = models.ForeignKey(Shard, models.DO_NOTHING, blank=True, null=True)
    flavor = models.ForeignKey(Flavor, models.DO_NOTHING, db_column='flavor')
    space_limit = models.BigIntegerField()
    fqdn = models.TextField()
    rev = models.BigIntegerField()
    vtype_id = models.TextField(blank=True, null=True)
    geo = models.ForeignKey(Geo, models.DO_NOTHING)
    disk_type = models.ForeignKey(DiskType, models.DO_NOTHING)
    subnet_id = models.TextField()
    assign_public_ip = models.BooleanField()
    created_at = models.DateTimeField()

    class Meta:
        managed = False
        db_table = 'hosts_revs'
        unique_together = (('fqdn', 'rev'),)


class DefaultPillar(models.Model):
    id = models.BigIntegerField(primary_key=True)
    value = JSONField()

    class Meta:
        managed = False
        db_table = 'default_pillar'


class ClusterTypePillar(models.Model):
    type = models.CharField(max_length=16, choices=[(x, x) for x in CLUSTER_TYPES])
    value = JSONField()

    class Meta:
        managed = False
        db_table = 'cluster_type_pillar'


class RolePillar(models.Model):
    type = models.CharField(max_length=16, choices=[(x, x) for x in CLUSTER_TYPES])
    role = models.CharField(max_length=16, choices=[(x, x) for x in ROLE_TYPES])
    value = JSONField()

    class Meta:
        managed = False
        db_table = 'role_pillar'
        unique_together = (('type', 'role'),)


class Pillar(models.Model):
    cid = models.OneToOneField(Cluster, models.DO_NOTHING, db_column='cid', blank=True, null=True)
    subcid = models.OneToOneField(Subcluster, models.DO_NOTHING, db_column='subcid', blank=True, null=True)
    shard = models.OneToOneField(Shard, models.DO_NOTHING, db_column='shard_id', blank=True, null=True)
    fqdn = models.OneToOneField(Host, models.DO_NOTHING, db_column='fqdn', blank=True, null=True)
    value = JSONField()

    class Meta:
        managed = False
        db_table = 'pillar'
        unique_together = (('cid', 'subcid', 'shard', 'fqdn'),)

    @staticmethod
    def get_value(**kwargs):
        pillars = Pillar.objects.filter(**kwargs).values('value')
        if pillars:
            return pillars[0]['value']


class PillarRev(models.Model):
    rev = models.BigIntegerField()
    cid = models.ForeignKey(Cluster, models.DO_NOTHING, db_column='cid', blank=True, null=True)
    subcid = models.ForeignKey(Subcluster, models.DO_NOTHING, db_column='subcid', blank=True, null=True)
    shard = models.ForeignKey(Shard, models.DO_NOTHING, blank=True, null=True)
    fqdn = models.ForeignKey(Host, models.DO_NOTHING, db_column='fqdn', blank=True, null=True)
    value = JSONField()

    class Meta:
        managed = False
        db_table = 'pillar_revs'
        unique_together = (
            ('cid', 'rev'),
            ('fqdn', 'rev'),
            ('shard', 'rev'),
            ('subcid', 'rev'),
        )

    @staticmethod
    def get_prev_rev(rev, **kwargs):
        pr = PillarRev.objects.filter(rev__lte=rev, **kwargs).order_by('-rev').values('rev', 'value').first()
        if pr:
            return pr

    @staticmethod
    def get_changes(pillar_revs, with_revs=True):
        changes = []
        for i in range(1, len(pillar_revs)):
            if with_revs:
                a = pillar_revs[i - 1]['value']
                b = pillar_revs[i]['value']
                result = list(diff(a, b))
                if result:
                    print('   chapson   1 ')
                    print(a)
                    print('   chapson   2 ')
                    print(b)
                    print('    chapson result   ')
                    print(result)
                changes.append((pillar_revs[i]['rev'], result))

            else:
                old = pillar_revs[i - 1].get('value', {}) if pillar_revs[i - 1] else {}
                new = pillar_revs[i].get('value', {}) if pillar_revs[i] else {}
                changes.append((list(diff(old, new))))
        return changes


class WorkerQueue(models.Model, LinkedMixin):
    task_id = models.TextField(primary_key=True)
    cid = models.ForeignKey(Cluster, models.DO_NOTHING, db_column='cid')
    create_ts = models.DateTimeField()
    start_ts = models.DateTimeField(blank=True, null=True)
    end_ts = models.DateTimeField(blank=True, null=True)
    worker_id = models.TextField(blank=True, null=True)
    task_type = models.TextField()
    task_args = JSONField()
    result = models.NullBooleanField()
    changes = JSONField()
    comment = models.TextField(blank=True, null=True)
    created_by = models.TextField(blank=True, null=True)
    folder = models.ForeignKey(Folder, models.DO_NOTHING)
    operation_type = models.TextField()
    metadata = JSONField()
    hidden = models.BooleanField()
    version = models.IntegerField()
    delayed_until = models.DateTimeField(blank=True, null=True)
    required_task = models.ForeignKey('self', models.DO_NOTHING, blank=True, null=True)
    errors = JSONField(blank=True, null=True)
    context = JSONField(blank=True, null=True)
    timeout = models.DurationField()
    create_rev = models.BigIntegerField()
    acquire_rev = models.BigIntegerField(blank=True, null=True)
    finish_rev = models.BigIntegerField(blank=True, null=True)
    unmanaged = models.BooleanField()
    tracing = models.TextField(blank=True, null=True)
    target_rev = models.BigIntegerField(blank=True, null=True)
    config_id = models.TextField(blank=True, null=True)
    restart_count = models.BigIntegerField(blank=True, default=0)

    class Meta:
        managed = False
        db_table = 'worker_queue'
        unique_together = (('create_ts', 'task_id'),)

    def __str__(self):
        return self.task_id

    def parse_shipments(self):
        shipment_ids = set()
        for row in self.comment.split('\n'):
            if 'deployv2.components' in row:
                substr = row[row.find('[') + 1 : row.find(']')]
                shipment_ids.update(re.findall(r'[. ](\d+)[,()]', substr))
        for ch in self.changes:
            for row in ch.keys():
                if row.startswith('deployv2'):
                    shipment_ids.add(row.split('.')[-1])
        return shipment_ids


class DefaultFeatureFlag(models.Model):
    flag_name = models.TextField(primary_key=True)

    class Meta:
        managed = False
        db_table = 'default_feature_flags'


class CloudFeatureFlag(models.Model):
    cloud = models.OneToOneField(Cloud, models.DO_NOTHING)
    flag_name = models.TextField()

    class Meta:
        managed = False
        db_table = 'cloud_feature_flags'
        unique_together = (('cloud', 'flag_name'),)


class ValidResource(models.Model):
    id = models.IntegerField(primary_key=True)
    cluster_type = models.CharField(max_length=16, choices=[(x, x) for x in CLUSTER_TYPES])
    role = models.CharField(max_length=16, choices=[(x, x) for x in ROLE_TYPES])
    flavor = models.ForeignKey(Flavor, models.DO_NOTHING, db_column='flavor')
    disk_type = models.ForeignKey(DiskType, models.DO_NOTHING)
    geo = models.ForeignKey(Geo, models.DO_NOTHING)
    disk_size_range = models.CharField(max_length=64, blank=True, null=True)
    disk_sizes = ArrayField(models.BigIntegerField())
    min_hosts = models.IntegerField()
    max_hosts = models.IntegerField()
    feature_flag = models.TextField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'valid_resources'
        unique_together = (('cluster_type', 'role', 'flavor', 'disk_type', 'geo'),)


class Version(models.Model):
    cid = models.ForeignKey(Cluster, models.DO_NOTHING, db_column='cid', related_name='versions', blank=True, null=True)
    subcid = models.ForeignKey(Subcluster, models.DO_NOTHING, db_column='subcid', blank=True, null=True)
    shard = models.ForeignKey(Shard, models.DO_NOTHING, blank=True, null=True)
    component = models.TextField()
    major_version = models.TextField()
    minor_version = models.TextField()
    package_version = models.TextField()
    edition = models.TextField()

    class Meta:
        managed = False
        db_table = 'versions'
        unique_together = (
            ('cid', 'component'),
            ('shard', 'component'),
            ('subcid', 'component'),
        )


class MaintenanceTask(models.Model):
    cid = models.OneToOneField(Cluster, models.DO_NOTHING, db_column='cid', primary_key=True)
    config_id = models.TextField()
    task_id = models.TextField(blank=True, null=True)
    create_ts = models.DateTimeField()
    plan_ts = models.DateTimeField(blank=True, null=True)
    status = models.CharField(max_length=16, choices=[(x, x) for x in MAINTENANCE_TASK_STATUSES])
    info = models.TextField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'maintenance_tasks'
        unique_together = (('cid', 'config_id'),)

    def __str__(self):
        return self.cid_id


class MaintenanceWindowSetting(models.Model):
    cid = models.OneToOneField(Cluster, models.DO_NOTHING, db_column='cid', primary_key=True)
    day = models.CharField(max_length=4, choices=[(x, x) for x in MAINTENANCE_WINDOW_DAYS])
    hour = models.IntegerField()

    class Meta:
        managed = False
        db_table = 'maintenance_window_settings'

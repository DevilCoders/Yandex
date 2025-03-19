from django.contrib.postgres.fields import JSONField
from django.db import models

from cloud.mdb.ui.internal.common import LinkedMixin

VOLUME_BACKENDS = [
    ('plain', 'plain'),
    ('bind', 'bind'),
    ('tmpfs', 'tmpfs'),
    ('quota', 'quota'),
    ('native', 'native'),
    ('overlay', 'overlay'),
    ('loop', 'loop'),
    ('rbd', 'rbd'),
]


class Cluster(models.Model, LinkedMixin):
    name = models.TextField(primary_key=True)
    project = models.ForeignKey('Project', models.DO_NOTHING, db_column='project')

    class Meta:
        managed = False
        db_table = 'clusters'

    def __str__(self):
        return self.name


class Container(models.Model, LinkedMixin):
    dom0 = models.ForeignKey('Dom0Host', models.DO_NOTHING, db_column='dom0', related_name='containers')
    fqdn = models.TextField(primary_key=True)
    cluster_name = models.ForeignKey(Cluster, models.DO_NOTHING, db_column='cluster_name')
    cpu_guarantee = models.FloatField(blank=True, null=True)
    cpu_limit = models.FloatField(blank=True, null=True)
    memory_guarantee = models.BigIntegerField(blank=True, null=True)
    memory_limit = models.BigIntegerField(blank=True, null=True)
    hugetlb_limit = models.BigIntegerField(blank=True, null=True)
    net_guarantee = models.BigIntegerField(blank=True, null=True)
    net_limit = models.BigIntegerField(blank=True, null=True)
    io_limit = models.BigIntegerField(blank=True, null=True)
    extra_properties = JSONField()
    bootstrap_cmd = models.TextField(blank=True, null=True)
    secrets = JSONField()
    secrets_expire = models.DateTimeField(blank=True, null=True)
    pending_delete = models.BooleanField()
    delete_token = models.UUIDField(blank=True, null=True)
    generation = models.IntegerField()

    class Meta:
        managed = False
        db_table = 'containers'
        unique_together = (('dom0', 'fqdn'),)

    def __str__(self):
        return self.fqdn


class Disk(models.Model):
    id = models.UUIDField(primary_key=True)
    max_space_limit = models.BigIntegerField()
    has_data = models.BooleanField()
    dom0 = models.ForeignKey('Dom0Host', models.DO_NOTHING, db_column='dom0')

    class Meta:
        managed = False
        db_table = 'disks'
        unique_together = (('dom0', 'id'),)

    def __str__(self):
        return str(self.id)


class Dom0Host(models.Model, LinkedMixin):
    fqdn = models.TextField(primary_key=True)
    project = models.ForeignKey('Project', models.DO_NOTHING, db_column='project')
    geo = models.ForeignKey('Location', models.DO_NOTHING, db_column='geo')
    cpu_cores = models.IntegerField()
    memory = models.BigIntegerField()
    ssd_space = models.BigIntegerField()
    sata_space = models.BigIntegerField()
    max_io = models.BigIntegerField()
    net_speed = models.BigIntegerField()
    allow_new_hosts = models.BooleanField()
    heartbeat = models.DateTimeField(blank=True, null=True)
    generation = models.IntegerField()
    use_vlan688 = models.BooleanField()
    allow_new_hosts_updated_by = models.TextField(blank=True, null=True)
    switch = models.TextField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'dom0_hosts'

    def __str__(self):
        return self.fqdn

    @property
    def resources_used(self):
        containers = list(self.containers.all())
        volumes = list(self.volumes.all())
        return {
            'cpu': sum([c.cpu_limit for c in containers]),
            'memory': sum([c.memory_limit or 0 + c.hugetlb_limit or 0 for c in containers]),
            'net': sum([c.net_limit for c in containers]),
            'io': sum([c.io_limit for c in containers]),
            'ssd': sum([v.ssd_used for v in volumes]),
            'sata': sum([v.sata_used for v in volumes]),
        }


class Location(models.Model):
    geo = models.TextField(primary_key=True)

    class Meta:
        managed = False
        db_table = 'locations'

    def __str__(self):
        return self.geo


class Project(models.Model):
    name = models.TextField(primary_key=True)
    description = models.TextField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'projects'

    def __str__(self):
        return self.name


class ReservedResource(models.Model):
    generation = models.IntegerField(primary_key=True)
    cpu_cores = models.IntegerField()
    memory = models.BigIntegerField()
    io = models.BigIntegerField()
    net = models.BigIntegerField()
    ssd_space = models.BigIntegerField()

    class Meta:
        managed = False
        db_table = 'reserved_resources'

    def __str__(self):
        return str(self.generation)


class Transfer(models.Model):
    id = models.TextField(primary_key=True)
    src_dom0 = models.ForeignKey(Container, models.DO_NOTHING, db_column='src_dom0', related_name='sources')
    dest_dom0 = models.ForeignKey(Container, models.DO_NOTHING, db_column='dest_dom0', related_name='dests')
    container = models.TextField(unique=True)
    placeholder = models.TextField()
    started = models.DateTimeField()

    class Meta:
        managed = False
        db_table = 'transfers'

    def __str__(self):
        return self.id


class Volume(models.Model):
    container = models.OneToOneField(Container, models.DO_NOTHING, db_column='container', primary_key=True)
    path = models.TextField()
    dom0 = models.ForeignKey(Dom0Host, models.DO_NOTHING, db_column='dom0', related_name='volumes')
    dom0_path = models.TextField()
    backend = models.CharField(max_length=8, choices=VOLUME_BACKENDS)
    read_only = models.BooleanField()
    space_guarantee = models.BigIntegerField(blank=True, null=True)
    space_limit = models.BigIntegerField(blank=True, null=True)
    inode_guarantee = models.BigIntegerField(blank=True, null=True)
    inode_limit = models.BigIntegerField(blank=True, null=True)
    pending_backup = models.BooleanField()
    disk = models.ForeignKey(Disk, models.DO_NOTHING, blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'volumes'
        unique_together = (
            ('container', 'path'),
            ('dom0', 'dom0_path'),
        )

    @property
    def ssd_used(self):
        if self.dom0_path.startswith('/data/'):
            if self.space_guarantee:
                return self.space_guarantee
            else:
                return self.space_limit or 0
        else:
            return 0

    @property
    def sata_used(self):
        if self.dom0_path.startswith('/slow/'):
            if self.space_guarantee:
                return self.space_guarantee
            else:
                return self.space_limit or 0
        else:
            return 0

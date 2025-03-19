import django.db

import cloud.mdb.backstage.lib.helpers as mod_helpers


class VolumeBackends:
    PLAIN = ('plain', 'Plain')
    BIND = ('bind', 'Bind')
    TMPFS = ('tmpfs', 'Tmpfs')
    QUOTA = ('quota', 'Quota')
    NATIVE = ('native', 'Native')
    OVERLAY = ('overlay', 'Overlay')
    LOOP = ('loop', 'Loop')
    RBD = ('rbd', 'Rbd')

    all = [
        PLAIN,
        BIND,
        TMPFS,
        QUOTA,
        NATIVE,
        OVERLAY,
        LOOP,
        RBD,
    ]


class Disk(django.db.models.Model):
    id = django.db.models.UUIDField(primary_key=True)
    max_space_limit = django.db.models.BigIntegerField()
    has_data = django.db.models.BooleanField()
    dom0 = django.db.models.ForeignKey('Dom0Host', django.db.models.DO_NOTHING, db_column='dom0')

    class Meta:
        managed = False
        db_table = 'disks'
        unique_together = (('dom0', 'id'),)

    def __str__(self):
        return str(self.id)


class Location(django.db.models.Model):
    geo = django.db.models.TextField(primary_key=True)

    class Meta:
        managed = False
        db_table = 'locations'

    def __str__(self):
        return self.geo


class ProjectUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .order_by('name')


class Project(django.db.models.Model, mod_helpers.LinkedMixin):
    name = django.db.models.TextField(primary_key=True)
    description = django.db.models.TextField(blank=True, null=True)

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = ProjectUiListManager()

    class Meta:
        managed = False
        db_table = 'projects'

    def __str__(self):
        return self.name


class Dom0HostUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('project')\
            .select_related('location')\
            .order_by('fqdn')


class Dom0HostUiObjManager(Dom0HostUiListManager):
    def get_queryset(self):
        return super().get_queryset()\
            .prefetch_related('containers')\
            .prefetch_related('containers__cluster')


class Dom0Host(django.db.models.Model, mod_helpers.LinkedMixin):
    fqdn = django.db.models.TextField(primary_key=True)
    project = django.db.models.ForeignKey(Project, django.db.models.DO_NOTHING, db_column='project')
    location = django.db.models.ForeignKey(Location, django.db.models.DO_NOTHING, db_column='geo')
    cpu_cores = django.db.models.IntegerField()
    memory = django.db.models.BigIntegerField()
    ssd_space = django.db.models.BigIntegerField()
    sata_space = django.db.models.BigIntegerField()
    max_io = django.db.models.BigIntegerField()
    net_speed = django.db.models.BigIntegerField()
    allow_new_hosts = django.db.models.BooleanField()
    heartbeat = django.db.models.DateTimeField(blank=True, null=True)
    generation = django.db.models.IntegerField()
    use_vlan688 = django.db.models.BooleanField()
    allow_new_hosts_updated_by = django.db.models.TextField(blank=True, null=True)
    switch = django.db.models.TextField(blank=True, null=True)

    objects = django.db.models.Manager()
    ui_obj = Dom0HostUiObjManager()
    ui_list = Dom0HostUiListManager()

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


class ReservedResourceUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .order_by('cpu_cores')


class ReservedResource(django.db.models.Model):
    generation = django.db.models.IntegerField(primary_key=True)
    cpu_cores = django.db.models.IntegerField()
    memory = django.db.models.BigIntegerField()
    io = django.db.models.BigIntegerField()
    net = django.db.models.BigIntegerField()
    ssd_space = django.db.models.BigIntegerField()

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = ReservedResourceUiListManager()

    class Meta:
        managed = False
        db_table = 'reserved_resources'

    def __str__(self):
        return str(self.generation)


class ClusterUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('project')\
            .order_by('name')


class ClusterUiObjManager(ClusterUiListManager):
    def get_queryset(self):
        return super().get_queryset()\
            .prefetch_related('container_set')


class Cluster(django.db.models.Model, mod_helpers.LinkedMixin):
    name = django.db.models.TextField(primary_key=True)
    project = django.db.models.ForeignKey(Project, django.db.models.DO_NOTHING, db_column='project')

    objects = django.db.models.Manager()
    ui_obj = ClusterUiObjManager()
    ui_list = ClusterUiListManager()

    class Meta:
        managed = False
        db_table = 'clusters'

    def __str__(self):
        return self.name

    @property
    def containers(self):
        return Container.ui_list.filter(cluster=self)


class ContainerUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('cluster')\
            .select_related('dom0host')\
            .order_by('fqdn')


class ContainerUiObjManager(ContainerUiListManager):
    def get_queryset(self):
        return super().get_queryset()\
            .prefetch_related('volume_set')


class Container(django.db.models.Model, mod_helpers.LinkedMixin):
    dom0host = django.db.models.ForeignKey(Dom0Host, django.db.models.DO_NOTHING, db_column='dom0', related_name='containers')
    fqdn = django.db.models.TextField(primary_key=True)
    cluster = django.db.models.ForeignKey(Cluster, django.db.models.DO_NOTHING, db_column='cluster_name')
    cpu_guarantee = django.db.models.FloatField(blank=True, null=True)
    cpu_limit = django.db.models.FloatField(blank=True, null=True)
    memory_guarantee = django.db.models.BigIntegerField(blank=True, null=True)
    memory_limit = django.db.models.BigIntegerField(blank=True, null=True)
    hugetlb_limit = django.db.models.BigIntegerField(blank=True, null=True)
    net_guarantee = django.db.models.BigIntegerField(blank=True, null=True)
    net_limit = django.db.models.BigIntegerField(blank=True, null=True)
    io_limit = django.db.models.BigIntegerField(blank=True, null=True)
    extra_properties = django.db.models.JSONField()
    bootstrap_cmd = django.db.models.TextField(blank=True, null=True)
    secrets = django.db.models.JSONField()
    secrets_expire = django.db.models.DateTimeField(blank=True, null=True)
    pending_delete = django.db.models.BooleanField()
    delete_token = django.db.models.UUIDField(blank=True, null=True)
    generation = django.db.models.IntegerField()

    objects = django.db.models.Manager()
    ui_obj = ContainerUiObjManager()
    ui_list = ContainerUiListManager()

    class Meta:
        managed = False
        db_table = 'containers'
        unique_together = (('dom0host', 'fqdn'),)

    def __str__(self):
        return self.fqdn


class TransferUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .order_by('-started')


class Transfer(django.db.models.Model):
    id = django.db.models.TextField(primary_key=True)
    src_dom0 = django.db.models.ForeignKey(Container, django.db.models.DO_NOTHING, db_column='src_dom0', related_name='sources')
    dest_dom0 = django.db.models.ForeignKey(Container, django.db.models.DO_NOTHING, db_column='dest_dom0', related_name='dests')
    container = django.db.models.TextField(unique=True)
    placeholder = django.db.models.TextField()
    started = django.db.models.DateTimeField()

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = TransferUiListManager()

    class Meta:
        managed = False
        db_table = 'transfers'

    def __str__(self):
        return self.id


class Volume(django.db.models.Model):
    container = django.db.models.ForeignKey(Container, django.db.models.DO_NOTHING, db_column='container')
    path = django.db.models.TextField()
    dom0 = django.db.models.ForeignKey(Dom0Host, django.db.models.DO_NOTHING, db_column='dom0', related_name='volumes')
    dom0_path = django.db.models.TextField(primary_key=True)  # fake primary key for ORM works
    backend = django.db.models.CharField(max_length=8, choices=VolumeBackends.all)
    read_only = django.db.models.BooleanField()
    space_guarantee = django.db.models.BigIntegerField(blank=True, null=True)
    space_limit = django.db.models.BigIntegerField(blank=True, null=True)
    inode_guarantee = django.db.models.BigIntegerField(blank=True, null=True)
    inode_limit = django.db.models.BigIntegerField(blank=True, null=True)
    disk = django.db.models.ForeignKey(Disk, django.db.models.DO_NOTHING, blank=True, null=True)

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

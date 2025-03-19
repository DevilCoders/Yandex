from django.contrib.postgres.fields import ArrayField, JSONField
from django.db import models

from cloud.mdb.ui.internal.common import LinkedMixin

MASTER_CHANGE_TYPES = [
    'create',
    'delete',
    'update',
    'add-alias',
    'remove-alias',
]

MINION_CHANGE_TYPES = [
    'create',
    'delete',
    'register',
    'reassign',
    'unregister',
]

SHIPMENT_STATUSES = [
    'INPROGRESS',
    'DONE',
    'ERROR',
    'TIMEOUT',
]

COMMAND_STATUSES = [
    'BLOCKED',
    'AVAILABLE',
    'RUNNING',
    'DONE',
    'ERROR',
    'CANCELED',
    'TIMEOUT',
]

JOB_STATUSES = [
    'RUNNING',
    'DONE',
    'ERROR',
    'TIMEOUT',
]

JOB_RESULT_STATUSES = [
    'SUCCESS',
    'FAILURE',
    'UNKNOWN',
    'TIMEOUT',
    'NOTRUNNING',
]


class Group(models.Model, LinkedMixin):
    group_id = models.IntegerField(primary_key=True)
    name = models.CharField(max_length=256, unique=True)

    class Meta:
        managed = False
        db_table = 'groups'

    def __str__(self):
        return self.name


class Master(models.Model, LinkedMixin):
    master_id = models.BigIntegerField(primary_key=True)
    group = models.ForeignKey(Group, models.DO_NOTHING)
    fqdn = models.CharField(max_length=256, unique=True)
    is_open = models.BooleanField()
    description = models.CharField(max_length=256, blank=True, null=True)
    created_at = models.DateTimeField()
    updated_at = models.DateTimeField()
    alive_check_at = models.DateTimeField()
    is_alive = models.BooleanField()

    class Meta:
        managed = False
        db_table = 'masters'
        unique_together = (('master_id', 'group'),)

    def __str__(self):
        return self.fqdn


class MasterAlias(models.Model):
    master = models.OneToOneField(Master, models.DO_NOTHING, primary_key=True)
    alias = models.TextField(unique=True)

    class Meta:
        managed = False
        db_table = 'master_aliases'

    def __str__(self):
        return self.alias


class MasterCheckView(models.Model):
    master = models.OneToOneField(Master, models.DO_NOTHING, primary_key=True)
    checker_fqdn = models.TextField()
    is_alive = models.BooleanField()
    updated_at = models.DateTimeField()

    class Meta:
        managed = False
        db_table = 'masters_check_view'
        unique_together = (('master', 'checker_fqdn'),)


class MasterChangeLog(models.Model):
    change_id = models.BigIntegerField(primary_key=True)
    master = models.ForeignKey(Master, models.DO_NOTHING)
    changed_at = models.DateTimeField()
    change_type = models.CharField(max_length=16, choices=[(x, x) for x in MASTER_CHANGE_TYPES])
    old_row = JSONField()
    new_row = JSONField()

    class Meta:
        managed = False
        db_table = 'masters_change_log'


class Minion(models.Model, LinkedMixin):
    minion_id = models.BigIntegerField(primary_key=True)
    fqdn = models.TextField(unique=True)
    group = models.ForeignKey(Group, models.DO_NOTHING)
    master = models.ForeignKey(Master, models.DO_NOTHING)
    pub_key = models.TextField(blank=True, null=True)
    auto_reassign = models.BooleanField()
    created_at = models.DateTimeField()
    updated_at = models.DateTimeField()
    register_until = models.DateTimeField(blank=True, null=True)
    deleted = models.BooleanField()

    class Meta:
        managed = False
        db_table = 'minions'

    def __str__(self):
        return self.fqdn


class MinionChangeLog(models.Model):
    change_id = models.BigIntegerField(primary_key=True)
    minion = models.ForeignKey(Minion, models.DO_NOTHING)
    changed_at = models.DateTimeField()
    change_type = models.CharField(max_length=16, choices=[(x, x) for x in MINION_CHANGE_TYPES])
    old_row = JSONField()
    new_row = JSONField()

    class Meta:
        managed = False
        db_table = 'minions_change_log'


class Shipment(models.Model, LinkedMixin):
    shipment_id = models.BigIntegerField(primary_key=True)
    fqdns = ArrayField(models.CharField(max_length=256, blank=True, null=True))
    status = models.CharField(max_length=16, choices=[(x, x) for x in SHIPMENT_STATUSES])
    parallel = models.BigIntegerField()
    stop_on_error_count = models.BigIntegerField()
    other_count = models.BigIntegerField()
    done_count = models.BigIntegerField()
    errors_count = models.BigIntegerField()
    total_count = models.BigIntegerField()
    created_at = models.DateTimeField()
    updated_at = models.DateTimeField()
    timeout = models.DurationField()
    tracing = models.TextField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'shipments'

    def __str__(self):
        return str(self.shipment_id)


class ShipmentCommand(models.Model, LinkedMixin):
    shipment_command_id = models.BigIntegerField(primary_key=True)
    shipment = models.ForeignKey(Shipment, models.DO_NOTHING)
    type = models.TextField()
    arguments = ArrayField(models.CharField(max_length=256, blank=True, null=True))
    timeout = models.DurationField()

    class Meta:
        managed = False
        db_table = 'shipment_commands'

    def __str__(self):
        return str(self.shipment_command_id)

    @property
    def pretty(self):
        return self.type + " " + " ".join(self.arguments)


class Command(models.Model):
    command_id = models.BigIntegerField(primary_key=True)
    minion = models.ForeignKey(Minion, models.DO_NOTHING)
    status = models.CharField(max_length=16, choices=[(x, x) for x in COMMAND_STATUSES])
    created_at = models.DateTimeField()
    updated_at = models.DateTimeField()
    shipment_command = models.ForeignKey(ShipmentCommand, models.DO_NOTHING, related_name='commands')
    last_dispatch_attempt_at = models.DateTimeField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'commands'

    def __str__(self):
        return str(self.command_id)


class Job(models.Model):
    job_id = models.BigIntegerField(primary_key=True)
    ext_job_id = models.CharField(max_length=256)
    command = models.ForeignKey(Command, models.DO_NOTHING, related_name='jobs')
    status = models.CharField(max_length=16, choices=[(x, x) for x in JOB_STATUSES])
    created_at = models.DateTimeField()
    updated_at = models.DateTimeField()
    last_running_check_at = models.DateTimeField()
    running_checks_failed = models.IntegerField()

    class Meta:
        managed = False
        db_table = 'jobs'

    def __str__(self):
        return self.ext_job_id

    @property
    def failed_states(self):
        jobresult = JobResult.objects.filter(ext_job_id=self.ext_job_id).first()
        if not jobresult:
            return
        return jobresult.parse_failed_states()


class JobResult(models.Model, LinkedMixin):
    job_result_id = models.BigIntegerField(primary_key=True)
    ext_job_id = models.CharField(max_length=256)
    fqdn = models.CharField(max_length=256)
    status = models.CharField(max_length=16, choices=[(x, x) for x in JOB_RESULT_STATUSES])
    result = JSONField()
    recorded_at = models.DateTimeField()
    order_id = models.IntegerField()

    class Meta:
        managed = False
        db_table = 'job_results'
        unique_together = (('ext_job_id', 'fqdn', 'order_id'),)

    def __str__(self):
        return self.ext_job_id

    def parse_failed_states(self):
        states = []
        if self.result.get('return'):
            for row in self.result['return'].values():
                if type(row) != dict:
                    continue
                if 'result' in row and not row['result']:
                    if 'comment' in row and 'One or more requisite failed' in row['comment']:
                        continue
                    if '__id__' in row:
                        states.append(row['__id__'])
        return states

import django.db
import django.utils.functional

import django.contrib.postgres.fields as pg_fields

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.apps.deploy.helpers as deploy_helpers


class MasterChangeType:
    CREATE = ('create', 'Create')
    DELETE = ('delete', 'Delete')
    UPDATE = ('update', 'Update')
    ADD_ALIAS = ('add-alias', 'Add alias')
    REMOVE_ALIAS = ('remove-alias', 'Remove alias')

    all = [
        CREATE,
        DELETE,
        UPDATE,
        ADD_ALIAS,
        REMOVE_ALIAS,
    ]


class MinionChangeType:
    CREATE = ('create', 'Create')
    DELETE = ('delete', 'Delete')
    REGISTER = ('register', 'Register')
    REASSIGN = ('reassign', 'Reassign')
    UNREGISTER = ('unregister', 'Unregister')

    all = [
        CREATE,
        DELETE,
        REGISTER,
        REASSIGN,
        UNREGISTER,
    ]


class ShipmentStatus:
    INPROGRESS = ('INPROGRESS', 'In progress')
    DONE = ('DONE', 'Done')
    ERROR = ('ERROR', 'Error')
    TIMEOUT = ('TIMEOUT', 'Timeout')

    all = [
        INPROGRESS,
        DONE,
        ERROR,
        TIMEOUT,
    ]


class CommandStatus:
    BLOCKED = ('BLOCKED', 'Blocked')
    AVAILABLE = ('AVAILABLE', 'Available')
    RUNNING = ('RUNNING', 'Running')
    DONE = ('DONE', 'Done')
    ERROR = ('ERROR', 'Error')
    CANCELED = ('CANCELED', 'Canceled')
    TIMEOUT = ('TIMEOUT', 'Timeout')

    all = [
        DONE,
        RUNNING,
        ERROR,
        AVAILABLE,
        BLOCKED,
        CANCELED,
        TIMEOUT,
    ]


class JobStatus:
    RUNNING = ('RUNNING', 'Running')
    DONE = ('DONE', 'Done')
    ERROR = ('ERROR', 'Error')
    TIMEOUT = ('TIMEOUT', 'Timeout')

    all = [
        RUNNING,
        DONE,
        ERROR,
        TIMEOUT,
    ]


class JobResultStatus:
    SUCCESS = ('SUCCESS', 'Success')
    FAILURE = ('FAILURE', 'Failure')
    UNKNOWN = ('UNKNOWN', 'Unknown')
    TIMEOUT = ('TIMEOUT', 'Timeout')
    NOTRUNNING = ('NOTRUNNING', 'Notrunning')

    all = [
        SUCCESS,
        FAILURE,
        UNKNOWN,
        TIMEOUT,
        NOTRUNNING,
    ]


class GroupUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .prefetch_related('master_set')


class GroupUiObjManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .prefetch_related('master_set')\
            .prefetch_related('minion_set')


class Group(django.db.models.Model, mod_helpers.LinkedMixin):
    group_id = django.db.models.IntegerField(primary_key=True)
    name = django.db.models.CharField(max_length=256, unique=True)

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = GroupUiListManager()

    class Meta:
        managed = False
        db_table = '"deploy"."groups"'

    def __str__(self):
        return str(self.name)

    def masters_url(self):
        return f'/ui/deploy/masters?group={self.name}'

    def minions_url(self):
        return f'/ui/deploy/minions?group={self.name}'


class MasterUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('group')\
            .order_by('-created_at')


class Master(django.db.models.Model, mod_helpers.LinkedMixin):
    master_id = django.db.models.BigIntegerField(primary_key=True)
    group = django.db.models.ForeignKey(Group, django.db.models.DO_NOTHING)
    fqdn = django.db.models.CharField(max_length=256, unique=True)
    is_open = django.db.models.BooleanField()
    description = django.db.models.CharField(max_length=256, blank=True, null=True)
    created_at = django.db.models.DateTimeField()
    updated_at = django.db.models.DateTimeField()
    alive_check_at = django.db.models.DateTimeField()
    is_alive = django.db.models.BooleanField()

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = MasterUiListManager()

    class Meta:
        managed = False
        db_table = '"deploy"."masters"'
        unique_together = (('master_id', 'group'),)

    def __str__(self):
        return str(self.fqdn)

    def minions_url(self):
        return f'/ui/deploy/minions?master={self.fqdn}'


class MasterAlias(django.db.models.Model):
    master = django.db.models.OneToOneField(Master, django.db.models.DO_NOTHING, primary_key=True)
    alias = django.db.models.TextField(unique=True)

    class Meta:
        managed = False
        db_table = '"deploy"."master_aliases"'

    def __str__(self):
        return self.alias


class MasterCheckView(django.db.models.Model):
    master = django.db.models.OneToOneField(Master, django.db.models.DO_NOTHING, primary_key=True)
    checker_fqdn = django.db.models.TextField()
    is_alive = django.db.models.BooleanField()
    updated_at = django.db.models.DateTimeField()

    class Meta:
        managed = False
        db_table = '"deploy"."masters_check_view"'
        unique_together = (('master', 'checker_fqdn'),)


class MasterChangeLog(django.db.models.Model):
    change_id = django.db.models.BigIntegerField(primary_key=True)
    master = django.db.models.ForeignKey(Master, django.db.models.DO_NOTHING)
    changed_at = django.db.models.DateTimeField()
    change_type = django.db.models.CharField(max_length=16, choices=MasterChangeType.all)
    old_row = django.db.models.JSONField()
    new_row = django.db.models.JSONField()

    class Meta:
        managed = False
        db_table = '"deploy"."masters_change_log"'


class MinionUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('group')\
            .select_related('master')\
            .order_by('-created_at')


class MinionUiObjManager(MinionUiListManager):
    pass


class Minion(django.db.models.Model, mod_helpers.LinkedMixin):
    minion_id = django.db.models.BigIntegerField(primary_key=True)
    fqdn = django.db.models.TextField(unique=True)
    group = django.db.models.ForeignKey(Group, django.db.models.DO_NOTHING)
    master = django.db.models.ForeignKey(Master, django.db.models.DO_NOTHING)
    pub_key = django.db.models.TextField(blank=True, null=True)
    auto_reassign = django.db.models.BooleanField()
    created_at = django.db.models.DateTimeField()
    updated_at = django.db.models.DateTimeField()
    register_until = django.db.models.DateTimeField(blank=True, null=True)
    deleted = django.db.models.BooleanField()

    objects = django.db.models.Manager()
    ui_obj = MinionUiObjManager()
    ui_list = MinionUiListManager()

    class Meta:
        managed = False
        db_table = '"deploy"."minions"'

    def __str__(self):
        return self.fqdn


class MinionChangeLog(django.db.models.Model):
    change_id = django.db.models.BigIntegerField(primary_key=True)
    minion = django.db.models.ForeignKey(Minion, django.db.models.DO_NOTHING)
    changed_at = django.db.models.DateTimeField()
    change_type = django.db.models.CharField(max_length=16, choices=MinionChangeType.all)
    old_row = django.db.models.JSONField()
    new_row = django.db.models.JSONField()

    class Meta:
        managed = False
        db_table = '"deploy"."minions_change_log"'


class ShipmentUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .order_by('-created_at')


class Shipment(django.db.models.Model, mod_helpers.LinkedMixin):
    shipment_id = django.db.models.BigIntegerField(primary_key=True)
    fqdns = pg_fields.ArrayField(
        django.db.models.TextField(blank=True)
    )
    status = django.db.models.CharField(max_length=16, choices=ShipmentStatus.all)
    parallel = django.db.models.BigIntegerField()
    stop_on_error_count = django.db.models.BigIntegerField()
    other_count = django.db.models.BigIntegerField()
    done_count = django.db.models.BigIntegerField()
    errors_count = django.db.models.BigIntegerField()
    total_count = django.db.models.BigIntegerField()
    created_at = django.db.models.DateTimeField()
    updated_at = django.db.models.DateTimeField()
    timeout = django.db.models.DurationField()
    tracing = django.db.models.TextField(blank=True, null=True)

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = ShipmentUiListManager()

    class Meta:
        managed = False
        db_table = '"deploy"."shipments"'

    def __str__(self):
        return str(self.shipment_id)


class ShipmentCommandUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('shipment')\
            .order_by('-shipment__created_at')


class ShipmentCommandUiObjManager(ShipmentCommandUiListManager):
    pass


class ShipmentCommand(django.db.models.Model, mod_helpers.LinkedMixin):
    shipment_command_id = django.db.models.BigIntegerField(primary_key=True)
    shipment = django.db.models.ForeignKey(Shipment, django.db.models.DO_NOTHING)
    type = django.db.models.TextField()
    arguments = pg_fields.ArrayField(
        django.db.models.CharField(max_length=256, blank=True, null=True)
    )
    timeout = django.db.models.DurationField()

    objects = django.db.models.Manager()
    ui_obj = ShipmentCommandUiObjManager()
    ui_list = ShipmentCommandUiListManager()

    class Meta:
        managed = False
        db_table = '"deploy"."shipment_commands"'

    def __str__(self):
        return str(self.shipment_command_id)

    @property
    def pretty(self):
        return self.type + " " + " ".join(self.arguments)

    @property
    def job_results(self):
        commands = self.commands.all()
        jobs = Job.objects.filter(command__in=commands)
        ext_job_ids = [j.ext_job_id for j in jobs]
        jobs_results = JobResult.objects.filter(ext_job_id__in=ext_job_ids)
        return jobs_results


class CommandUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .select_related('minion')\
            .select_related('shipment_command')\
            .order_by('-created_at')


class CommandUiObjManager(CommandUiListManager):
    def get_queryset(self):
        return super().get_queryset()\
            .prefetch_related('jobs')


class Command(django.db.models.Model, mod_helpers.LinkedMixin):
    command_id = django.db.models.BigIntegerField(primary_key=True)
    minion = django.db.models.ForeignKey(Minion, django.db.models.DO_NOTHING)
    status = django.db.models.CharField(max_length=16, choices=CommandStatus.all)
    created_at = django.db.models.DateTimeField()
    updated_at = django.db.models.DateTimeField()
    shipment_command = django.db.models.ForeignKey(ShipmentCommand, django.db.models.DO_NOTHING, related_name='commands')
    last_dispatch_attempt_at = django.db.models.DateTimeField(blank=True, null=True)

    objects = django.db.models.Manager()
    ui_obj = CommandUiObjManager()
    ui_list = CommandUiListManager()

    class Meta:
        managed = False
        db_table = '"deploy"."commands"'

    def __str__(self):
        return str(self.command_id)


class Job(django.db.models.Model, mod_helpers.LinkedMixin):
    job_id = django.db.models.BigIntegerField(primary_key=True)
    ext_job_id = django.db.models.CharField(max_length=256)
    command = django.db.models.ForeignKey(Command, django.db.models.DO_NOTHING, related_name='jobs')
    status = django.db.models.CharField(max_length=16, choices=JobStatus.all)
    created_at = django.db.models.DateTimeField()
    updated_at = django.db.models.DateTimeField()
    last_running_check_at = django.db.models.DateTimeField()
    running_checks_failed = django.db.models.IntegerField()

    class Meta:
        managed = False
        db_table = '"deploy"."jobs"'

    def __str__(self):
        return self.ext_job_id

    @property
    def failed_states(self):
        jobresult = JobResult.objects.filter(ext_job_id=self.ext_job_id).first()
        if not jobresult:
            return
        return jobresult.parse_failed_states()

    @property
    def job_result(self):
        return JobResult.objects.filter(ext_job_id=self.ext_job_id, fqdn=self.command.minion.fqdn).first()


class JobResultUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .defer('result')\
            .order_by('-recorded_at')


class JobResult(django.db.models.Model, mod_helpers.LinkedMixin):
    job_result_id = django.db.models.BigIntegerField(primary_key=True)
    ext_job_id = django.db.models.CharField(max_length=256)
    fqdn = django.db.models.CharField(max_length=256)
    status = django.db.models.CharField(max_length=16, choices=JobResultStatus.all)
    result = django.db.models.JSONField()
    recorded_at = django.db.models.DateTimeField()
    order_id = django.db.models.IntegerField()

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = JobResultUiListManager()

    class Meta:
        managed = False
        db_table = '"deploy"."job_results"'
        unique_together = (('ext_job_id', 'fqdn', 'order_id'),)

    def __str__(self):
        return str(self.job_result_id)

    def parse_failed_states(self):
        states = []
        if self.result:
            _return = self.result.get('return')
            if _return and isinstance(_return, dict):
                for row in self.result['return'].values():
                    if not isinstance(row, dict):
                        continue
                    if 'result' in row and not row['result']:
                        if 'comment' in row and 'One or more requisite failed' in row['comment']:
                            continue
                        if '__id__' in row:
                            states.append(row['__id__'])
        return states

    def parsed_result(self):
        return deploy_helpers.parse_salt_result(self.result)

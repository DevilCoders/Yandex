from django.contrib.postgres.fields import ArrayField
from django.db import models
from modelchoices import Choices


class Request(models.Model):
    id = models.IntegerField(primary_key=True)
    is_deleted = models.BooleanField()
    request_ext_id = models.TextField(max_length=100)
    name = models.CharField(max_length=256, help_text='what walle wants to do')
    status = models.CharField(max_length=256, help_text='what walle sees')
    fqdns = ArrayField(models.TextField(max_length=100))
    author = models.TextField(max_length=100)
    request_type = models.TextField(max_length=100)
    comment = models.TextField(max_length=100)
    created_at = models.DateTimeField()
    resolved_at = models.DateTimeField(null=True)
    came_back_at = models.DateTimeField(null=True)
    done_at = models.DateTimeField(null=True)
    resolved_by = models.TextField()
    analysed_by = models.TextField()
    resolve_explanation = models.TextField()

    class Meta:
        managed = False
        db_table = 'requests'

    def __str__(self) -> str:
        return str(self.name)


class DecisionManager(models.Manager):
    def get_queryset(self) -> models.QuerySet:
        return super().get_queryset().prefetch_related('request')  # type: ignore


class DecisionStatuses(Choices):
    escalated = ('escalated', 'Escalated to human')
    new = ('new', 'Awaits analysis')
    processing = ('processing', 'Processing')
    wait = ('wait', 'Waiting')
    ok = ('ok', 'Prepared to let go')
    at_walle = ('at-wall-e', 'Given away')
    before_done = ('before-done', 'Wall-e gave back')
    done = ('done', 'Terminal successful stage')
    rejected = ('rejected', 'Will NOT give away')
    cleanup = ('cleanup', 'Need clean')


class Decision(models.Model):
    id = models.IntegerField(primary_key=True)
    status = models.TextField(choices=DecisionStatuses.CHOICES, help_text='autoduty decision status')
    cleanup_log = models.TextField(name='cleanup_log', help_text='autoduty after dom0 came back')
    after_walle_log = models.TextField(name='after_walle_log', help_text='autoduty after dom0 came back')
    let_go_log = models.TextField(name='mutations_log', help_text='autoduty is giving away')
    analysis = models.TextField(name='explanation', help_text='autoduty before giving away')
    ad_resolution = models.TextField()
    request = models.OneToOneField(
        Request,
        on_delete=models.CASCADE,
        related_name='decision',
    )

    class Meta:
        managed = False
        db_table = 'decisions'

    objects = DecisionManager()

    def __str__(self) -> str:
        return str(self.id)


class InstanceOperationType(Choices):
    move = ('move', 'Resetup')
    whip_primary_away = ('whip_primary_away', 'Whip primary away')


class InstanceOperationStatus(Choices):
    new = ('new', 'New')
    in_progress = ('in-progress', 'In progress')
    ok_pending = ('ok-pending', 'OK pending')
    ok = ('ok', 'OK')
    reject_pending = ('reject-pending', 'Reject pending')
    rejected = ('rejected', 'Error')


class InstanceOperation(models.Model):
    class Meta:
        managed = False
        db_table = 'instance_operations'

    operation_id = models.UUIDField(primary_key=True)
    operation_type = models.CharField(choices=InstanceOperationType.CHOICES, name='operation_type', max_length=100)
    status = models.CharField(choices=InstanceOperationStatus.CHOICES, name='status', max_length=100)
    author = models.CharField(max_length=100)
    instance_id = models.CharField(max_length=200)
    created_at = models.DateTimeField()
    modified_at = models.DateTimeField()
    operation_log = models.TextField()
    comment = models.TextField()

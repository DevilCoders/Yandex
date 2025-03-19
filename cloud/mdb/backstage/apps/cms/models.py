from datetime import timedelta

import django.db
import django.utils.timezone as timezone
import django.contrib.postgres.fields as pg_fields

import cloud.mdb.backstage.lib.search as mod_search
import cloud.mdb.backstage.lib.helpers as mod_helpers


class DecisionAction:
    APPROVE = ('approve', 'Approve')
    MARK_AS_DONE = ('mark_as_done', 'Mark as done')
    OK_AT_WALLE = ('ok_at_walle', 'OK -> At Wall-E')

    all = [
        APPROVE,
        MARK_AS_DONE,
        OK_AT_WALLE,
    ]

    map = {
        APPROVE[0]: APPROVE,
        MARK_AS_DONE[0]: MARK_AS_DONE,
        OK_AT_WALLE[0]: OK_AT_WALLE,
    }


class InstanceOperationAction:
    STATUS_TO_IN_PROGRESS = ('status_to_in_progress', 'Status -> In Progress')
    STATUS_TO_OK = ('status_to_ok', 'Status -> OK')

    all = [
        STATUS_TO_IN_PROGRESS,
        STATUS_TO_OK,
    ]

    map = {
        STATUS_TO_IN_PROGRESS[0]: STATUS_TO_IN_PROGRESS,
        STATUS_TO_OK[0]: STATUS_TO_OK,
    }


class DecisionDutyRoutine:
    ESCALATED = ('escalaed', 'Escalated to human')
    GIVING_AWAY = ('giving_away', 'Giving away right now')
    RETURNING = ('returning', 'Given back, not finished yet')
    STALE = ('stale', 'Stale (not done too long)')

    all = [
        ESCALATED,
        GIVING_AWAY,
        RETURNING,
        STALE,
    ]


class InstanceOperationStatus:
    NEW = ('new', 'New')
    IN_PROGRESS = ('in-progress', 'In progress')
    OK_PENDING = ('ok-pending', 'OK pending')
    OK = ('ok', 'OK')
    REJECT_PENDING = ('reject-pending', 'Reject pending')
    REJECTED = ('rejected', 'Rejected')

    all = [
        NEW,
        IN_PROGRESS,
        OK_PENDING,
        OK,
        REJECT_PENDING,
        REJECTED,
    ]


class InstanceOperationType:
    MOVE = ('move', 'Move')
    WHIP_PRIMARY_AWAY = ('whip_primary_away', 'Whip primary away')

    all = [
        MOVE,
        WHIP_PRIMARY_AWAY,
    ]


class RequestStatus:
    OK = ('ok', 'OK')

    all = [OK]


class DecisionStatus:
    NEW = ('new', 'New')
    PROCESSING = ('processing', 'Processing')
    REJECTED = ('rejected', 'Rejected')
    ESCALATED= ('escalated', 'Escalated')
    OK = ('ok', 'Prepared to let go')
    AT_WALLE = ('at-wall-e', 'At Wall-E')
    WAIT = ('wait', 'Waiting')
    BEFORE_DONE = ('before-done', 'Wall-E gave back')
    DONE = ('done', 'Done')
    CLEANUP = ('cleanup', 'Cleanup')

    all = [
        NEW,
        PROCESSING,
        REJECTED,
        ESCALATED,
        OK,
        AT_WALLE,
        WAIT,
        BEFORE_DONE,
        CLEANUP,
        DONE,
    ]
    approvable = [
        ESCALATED[0],
        NEW[0],
        WAIT[0],
        PROCESSING[0],
    ]


class Request(django.db.models.Model):
    id = django.db.models.IntegerField(primary_key=True)
    is_deleted = django.db.models.BooleanField()
    request_ext_id = django.db.models.TextField(max_length=100)
    name = django.db.models.CharField(max_length=256)
    status = django.db.models.CharField(max_length=256)
    fqdns = pg_fields.ArrayField(django.db.models.TextField(max_length=100))
    author = django.db.models.TextField(max_length=100)
    request_type = django.db.models.TextField(max_length=100)
    comment = django.db.models.TextField(max_length=100)
    created_at = django.db.models.DateTimeField()
    resolved_at = django.db.models.DateTimeField(null=True)
    came_back_at = django.db.models.DateTimeField(null=True)
    done_at = django.db.models.DateTimeField(null=True)
    resolved_by = django.db.models.TextField()
    analysed_by = django.db.models.TextField()
    resolve_explanation = django.db.models.TextField()

    class Meta:
        managed = False
        db_table = 'requests'

    def __str__(self):
        return str(self.name)

    @property
    def related_links(self):
        result = []
        for fqdn in self.fqdns:
            result.extend(mod_search.search_by_fqdn_links(fqdn))
        return result


class DecisionUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset()\
            .prefetch_related('request')\
            .annotate(
                duty_routine=django.db.models.Case(
                    django.db.models.When(
                        status=DecisionStatus.ESCALATED[0],
                        request__is_deleted=False,
                        then=django.db.models.Value(DecisionDutyRoutine.ESCALATED[0]),
                    ),
                    django.db.models.When(
                        request__resolved_at__isnull=True,
                        request__is_deleted=False,
                        then=django.db.models.Value(DecisionDutyRoutine.GIVING_AWAY[0]),
                    ),
                    django.db.models.When(
                        status=DecisionStatus.BEFORE_DONE[0],
                        then=django.db.models.Value(DecisionDutyRoutine.RETURNING[0]),
                    ),
                    django.db.models.When(
                        request__resolved_at__lt=timezone.now() - timedelta(days=1),
                        request__is_deleted=False,
                        then=django.db.models.Value(DecisionDutyRoutine.STALE[0]),
                    ),
                    output_field=django.db.models.CharField()
                ),
            )\
            .order_by('-id')


class Decision(django.db.models.Model, mod_helpers.LinkedMixin):
    id = django.db.models.IntegerField(
        primary_key=True,
    )
    status = django.db.models.TextField(
        choices=DecisionStatus.all,
    )
    cleanup_log = django.db.models.TextField(
        name='cleanup_log',
    )
    after_walle_log = django.db.models.TextField(
        name='after_walle_log',
    )
    let_go_log = django.db.models.TextField(
        name='mutations_log',
    )
    ops_metadata_log = django.db.models.JSONField()
    analysis = django.db.models.TextField(
        name='explanation',
    )
    ad_resolution = django.db.models.TextField()
    request = django.db.models.OneToOneField(
        Request,
        on_delete=django.db.models.CASCADE,
        related_name='decision',
    )

    class Meta:
        managed = False
        db_table = 'decisions'

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = DecisionUiListManager()

    def __str__(self):
        return str(self.id)

    @property
    def fqdns(self):
        try:
            return ', '.join(self.request.fqdns).strip()
        except Exception:
            return None

    @property
    def walle_url(self):
        if self.request.fqdns:
            if len(self.request.fqdns) > 1:
                fqdn= ' '.join(self.rqeuest.fqdns)
                return f'https://wall-e.yandex-team.ru/projects/hosts?fqdn={fqdn}'
            else:
                fqdn = self.request.fqdns[0]
                return f'https://wall-e.yandex-team.ru/host/{fqdn}'
        else:
            return None

    def action_ability(self, action, **kwargs):
        if action == DecisionAction.APPROVE[0]:
            if self.status not in DecisionStatus.approvable:
                return False, f'invalid status for action "{DecisionAction.APPROVE[1]}"'
            else:
                return True, None
        elif action == DecisionAction.MARK_AS_DONE[0]:
            if self.status in [DecisionStatus.CLEANUP[0], DecisionStatus.DONE[0]]:
                return False, f'invalid status for action {DecisionAction.MARK_AS_DONE[1]}"'
            else:
                return True, None
        elif action == DecisionAction.OK_AT_WALLE[0]:
            if self.status != DecisionStatus.OK[0]:
                return False, f'invalid status for action {DecisionAction.OK_AT_WALLE[1]}'
        else:
            return False, f'uknown action: {action}'


class InstanceOperationUiListManager(django.db.models.Manager):
    def get_queryset(self):
        return super().get_queryset().order_by('-modified_at')


class InstanceOperation(django.db.models.Model, mod_helpers.LinkedMixin):
    author = django.db.models.CharField(
        max_length=100,
    )
    comment = django.db.models.TextField()
    created_at = django.db.models.DateTimeField()
    instance_id = django.db.models.CharField(
        max_length=200,
    )
    modified_at = django.db.models.DateTimeField()
    operation_id = django.db.models.UUIDField(
        primary_key=True,
    )
    operation_log = django.db.models.TextField()
    operation_type = django.db.models.CharField(
        choices=InstanceOperationType.all,
        max_length=100,
    )
    operation_state = django.db.models.JSONField()
    status = django.db.models.CharField(
        choices=InstanceOperationStatus.all,
        max_length=100,
    )

    objects = django.db.models.Manager()
    ui_obj = django.db.models.Manager()
    ui_list = InstanceOperationUiListManager()

    class Meta:
        managed = False
        db_table = 'instance_operations'

    def __str__(self):
        return str(self.operation_id)

    @property
    def fqdn(self):
        fqdn = self.operation_state.get('fqdn')
        if not fqdn:
            try:
                if ':' in self.instance_id:
                    _, fqdn = self.instance_id.split(':')
            except Exception:
                pass
        return fqdn

    def action_ability(self, action, **kwargs):
        if action == InstanceOperationAction.STATUS_TO_IN_PROGRESS[0]:
            if self.status == InstanceOperationStatus.IN_PROGRESS[0]:
                return False, f'invalid status for action "{InstanceOperationAction.STATUS_TO_IN_PROGRESS[1]}"'
            else:
                return True, None
        elif action == InstanceOperationAction.STATUS_TO_OK[0]:
            if self.status in [InstanceOperationStatus.OK[0], InstanceOperationStatus.OK_PENDING[0]]:
                return False, f'invalid status for action "{InstanceOperationAction.STATUS_TO_OK[1]}"'
            else:
                return True, None
        else:
            return False, f'uknown action: {action}'

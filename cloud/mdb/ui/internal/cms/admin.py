from datetime import datetime, timedelta
from enum import Enum
from typing import Optional

from django.contrib import admin
from django.utils.safestring import mark_safe
from django_object_actions import DjangoObjectActions

from cloud.mdb.ui.internal.common import ROAdmin, ROStackedInline
from cloud.mdb.ui.internal.omnisearch import search_by_fqdn

from . import models as cms_models
from .logic.at_walle import from_ok_to_at_walle
from .logic.clean_decisions import clean_decisions
from .logic.approve_requests import approve_requests
from .logic import change_operation_status


class RequestInline(ROStackedInline):
    model = cms_models.Request  # type: ignore
    fields = [  # type: ignore
        'name',
        'fqdns',
        'request_type',
        'created_at',
        'resolved_at',
        'came_back_at',
        'done_at',
        'id',
    ]


def walle_link(obj: cms_models.Decision):
    if obj:
        return mark_safe(
            '<a href="https://wall-e.yandex-team.ru/host/{fqdn}" target="_blank">{action} for {fqdns} by {author}</a>'.format(
                fqdn=obj.request.fqdns[0],
                action=obj.request.name,
                fqdns=','.join(obj.request.fqdns),
                author=obj.request.author,
            )
        )


walle_link.short_description = 'Link to Wall-e'


def request_fqdn(obj: cms_models.Decision):
    if obj:
        return ' '.join(obj.request.fqdns)


def request_comment(obj: cms_models.Decision):
    if obj:
        return obj.request.comment


def request_created_at(obj: cms_models.Decision):
    if obj:
        return obj.request.created_at


request_created_at.short_description = 'created at'


def request_let_go_at(obj: cms_models.Decision):
    if obj:
        return obj.request.resolved_at


request_let_go_at.short_description = 'given away at'


def request_let_go_duration(obj: cms_models.Decision) -> Optional[timedelta]:
    if obj:
        if obj.request.resolved_at:
            return datetime.now() - obj.request.resolved_at  # type: ignore
    return None


request_let_go_duration.short_description = 'given away'


def request_came_back_at(obj: cms_models.Decision):
    if obj:
        return obj.request.came_back_at


request_came_back_at.short_description = 'came back at'


def request_done_at(obj: cms_models.Decision):
    if obj:
        return obj.request.done_at


request_done_at.short_description = 'done at'


def request_done_completely(obj: cms_models.Decision) -> Optional[bool]:
    if obj:
        return obj.request.done_at is not None
    return None


class DutyRoutineNames(Enum):
    escalated = 'Escalated to human'
    giving_away = 'Giving away right now'
    returning = 'Given back, not finished yet'
    stale = 'Stale (not done too long)'


class DutyRoutineDecisionFilter(admin.SimpleListFilter):
    title = 'Duty routine'

    parameter_name = 'routine_type'

    def lookups(self, request, model_admin):
        """
        Returns a list of tuples. The first element in each
        tuple is the coded value for the option that will
        appear in the URL query. The second element is the
        human-readable name for the option that will appear
        in the right sidebar.
        """
        return [
            [item.name, item.value]
            for item in [
                DutyRoutineNames.escalated,
                DutyRoutineNames.giving_away,
                DutyRoutineNames.returning,
                DutyRoutineNames.stale,
            ]
        ]

    def queryset(self, request, queryset):
        """
        Returns the filtered queryset based on the value
        provided in the query string and retrievable via
        `self.value()`.
        """
        value = self.value()
        if value is None:
            return queryset
        elif value == DutyRoutineNames.escalated.name:
            return queryset.filter(
                status=cms_models.DecisionStatuses.escalated,
                request__is_deleted=False,
            )
        elif value == DutyRoutineNames.giving_away.name:
            return queryset.filter(
                request__resolved_at__isnull=True,
                request__is_deleted=False,
            )
        elif value == DutyRoutineNames.returning.name:
            return queryset.filter(
                status=cms_models.DecisionStatuses.before_done,
            )
        elif value == DutyRoutineNames.stale.name:
            return queryset.filter(
                request__resolved_at__lt=datetime.now() - timedelta(days=1),
                request__is_deleted=False,
            )
        else:
            raise RuntimeError('unknown value {}'.format(value))


class DecisionsAdmin(DjangoObjectActions, ROAdmin):
    inlines = []
    model = cms_models.Decision
    search_fields = [
        'request__fqdns',
        'id',
        'request__request_ext_id',
    ]
    ordering = [
        '-id',
    ]

    def approve_request_action(self, request, queryset):
        return approve_requests(list(queryset), by_user=request.user.username)

    approve_request_action.short_description = 'Approve this request'

    def approve_request_action_single(self, request, obj):
        return approve_requests([obj], by_user=request.user.username)

    approve_request_action_single.label = 'Approve'
    approve_request_action_single.short_description = 'Approve this request'

    def clean_decision_action_single(self, _, obj):
        return clean_decisions([obj])

    clean_decision_action_single.label = 'Mark as Done'
    clean_decision_action_single.short_description = 'Mark this as "done" and clean state'

    def to_at_walle_action_single(self, request, obj):
        return from_ok_to_at_walle([obj], by_user=request.user.username)

    to_at_walle_action_single.label = 'OK -> at-walle'
    to_at_walle_action_single.short_description = (
        'Mark this as "at-walle", when some shipment failed and you fixed the problem'
    )

    change_actions = [
        'approve_request_action_single',
        'clean_decision_action_single',
        'to_at_walle_action_single',
    ]

    def clean_decision_action(self, request, queryset):
        return clean_decisions(list(queryset))

    clean_decision_action.short_description = 'Start clean this decision'

    actions = [
        approve_request_action,
        clean_decision_action,
    ]

    list_display = [
        request_fqdn,
        'status',
        request_done_at,
        request_came_back_at,
        request_let_go_duration,
        request_created_at,
        request_done_completely,
        walle_link,
        'id',
    ]
    list_filter = [
        DutyRoutineDecisionFilter,
        'status',
    ]

    fields = [
        walle_link,
        request_done_completely,
        'status',
        'ad_resolution',
        request_done_at,
        'cleanup_log',
        'after_walle_log',
        request_came_back_at,
        'mutations_log',
        request_let_go_at,
        'explanation',
        request_created_at,
        request_comment,
        request_fqdn,
        'id',
        'links',
    ]

    def links(self, obj):
        if obj:
            links = []
            for fqdn in obj.request.fqdns:
                links.extend(search_by_fqdn(fqdn))
            return mark_safe('<br />'.join(links))


def operation_author(obj: cms_models.InstanceOperation):
    if obj:
        return obj.author


def instance_by_id(obj: cms_models.InstanceOperation):
    if obj:
        return obj.instance_id


class KnownUsers(Enum):
    cms = 'yc.mdb.cms'


class InstanceOperationsByUserFilter(admin.SimpleListFilter):
    title = 'user'
    MDB_CMS = 'yc.mdb.cms'

    parameter_name = 'by_author'

    def lookups(self, request, model_admin: cms_models.InstanceOperation):
        result = []
        for enum_item in KnownUsers:
            result.append((enum_item.name, enum_item.value))
        return result

    def queryset(self, request, queryset):
        value = self.value()
        if value is None:
            return queryset
        try:
            known_user = KnownUsers(value)
        except ValueError:
            return queryset.filter(author=value)  # search by unknown value.
        else:
            return queryset.filter(author=known_user.value)


class InstancesAdmin(DjangoObjectActions, ROAdmin):
    inlines = []
    model = cms_models.InstanceOperation
    search_fields = [
        'operation_id',
        'author',
        'instance_id',
    ]
    ordering = [
        '-created_at',
    ]

    def mark_successful(self, request, queryset):
        change_operation_status.mark_operation_ok(queryset)

    mark_successful.short_description = 'Set this operation as successful'

    def mark_successful_single(self, request, obj):
        return change_operation_status.mark_operation_ok([obj])

    mark_successful_single.label = 'Mark OK'
    mark_successful_single.short_description = 'Mark this successful'

    def move_to_progress(self, request, queryset):
        change_operation_status.move_operation_to_progress(queryset)

    def move_to_progress_single(self, request, obj):
        return change_operation_status.move_operation_to_progress([obj])

    move_to_progress_single.label = 'In progress'
    move_to_progress_single.short_description = 'Move to "in progress"'

    move_to_progress.short_description = 'Move this operation to progress'

    actions = [
        mark_successful,
        move_to_progress,
    ]

    change_actions = [
        'mark_successful_single',
        'move_to_progress_single',
    ]

    list_display = [
        'operation_id',
        instance_by_id,
        'created_at',
        'modified_at',
        'operation_type',
        'status',
        operation_author,
        'comment',
    ]
    list_filter = [
        InstanceOperationsByUserFilter,
        'status',
        'operation_type',
    ]

    fields = [
        'operation_id',
        'operation_type',
        'status',
        'author',
        'comment',
        'instance_id',
        'created_at',
        'modified_at',
        'operation_log',
    ]


class RequestsAdmin(ROAdmin):
    model = cms_models.Request
    list_display = [
        'name',
        'fqdns',
        'request_type',
        'created_at',
        'resolved_at',
        'came_back_at',
        'done_at',
        'id',
    ]
    fields = [
        'name',
        'fqdns',
        'author',
        'comment',
        'status',
        'created_at',
        'resolved_at',
        'came_back_at',
        'done_at',
        'id',
    ]
    search_fields = [
        'name',
    ]


admin.site.register(cms_models.Decision, DecisionsAdmin)
admin.site.register(cms_models.InstanceOperation, InstancesAdmin)

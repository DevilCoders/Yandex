from typing import List

from django.db import transaction

from .. import models as cms_models


@transaction.atomic(using='cms_primary')
def approve_requests(decisions: List[cms_models.Decision], by_user: str) -> None:
    decision_ids = {d.id: d for d in decisions}
    request_ids = {d.request_id for d in decisions}
    cms_models.Decision.objects.filter(
        id__in=decision_ids,
        status__in={
            cms_models.DecisionStatuses.escalated,
            cms_models.DecisionStatuses.new,
            cms_models.DecisionStatuses.wait,
            cms_models.DecisionStatuses.processing,
        },
    ).update(
        status=cms_models.DecisionStatuses.ok,
    )
    cms_models.Request.objects.filter(id__in=request_ids).update(
        analysed_by=by_user,
    )

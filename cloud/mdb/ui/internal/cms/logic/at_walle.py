from typing import List
from datetime import datetime

from django.db import transaction

from .. import models as cms_models

OK = 'ok'


@transaction.atomic(using='cms_primary')
def from_ok_to_at_walle(decisions: List[cms_models.Decision], by_user: str) -> None:
    """
        ok -> at-walle:
    UPDATE cms.requests SET status = 'ok', resolved_at = now(), resolved_by='duty' WHERE id = 11586;
    UPDATE cms.decisions SET status = 'at-wall-e' WHERE request_id = 11586;
    """
    decision_ids = {d.id: d for d in decisions}
    d_to_update = cms_models.Decision.objects.filter(
        id__in=decision_ids,
        status__in={cms_models.DecisionStatuses.ok},
    )
    request_ids = {d.request_id for d in d_to_update}
    d_to_update.update(
        status=cms_models.DecisionStatuses.at_walle,
    )
    cms_models.Request.objects.filter(id__in=request_ids).update(
        status=OK, resolved_by=by_user, resolved_at=datetime.now()
    )

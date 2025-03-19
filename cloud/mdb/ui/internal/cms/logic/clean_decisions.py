from typing import List

from django.db import transaction

from .. import models as cms_models


@transaction.atomic(using='cms_primary')
def clean_decisions(decisions: List[cms_models.Decision]) -> None:
    decision_ids = {d.id for d in decisions}
    cms_models.Decision.objects.filter(id__in=decision_ids,).update(
        status=cms_models.DecisionStatuses.cleanup,
    )

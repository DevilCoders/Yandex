from typing import List

from django.db import transaction

from .. import models as cms_models


@transaction.atomic(using='cms_primary')
def change_status(operations: List[cms_models.InstanceOperation], status: cms_models.InstanceOperationStatus) -> None:
    operation_ids = {o.operation_id for o in operations}
    cms_models.InstanceOperation.objects.filter(operation_id__in=operation_ids,).update(
        status=status,
    )


def mark_operation_ok(operations: List[cms_models.InstanceOperation]) -> None:
    change_status(operations, cms_models.InstanceOperationStatus.ok_pending)


def move_operation_to_progress(operations: List[cms_models.InstanceOperation]) -> None:
    change_status(operations, cms_models.InstanceOperationStatus.in_progress)

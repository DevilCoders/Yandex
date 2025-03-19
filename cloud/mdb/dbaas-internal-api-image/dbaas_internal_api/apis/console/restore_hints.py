# coding: utf-8
"""
DBaaS Internal API cluster restore from backups hints API
"""

from flask.views import MethodView

from .. import API
from ...core.auth import check_auth
from ...core.exceptions import DbaasNotImplementedError, UnsupportedHandlerError
from ...utils import config
from ...utils.cluster.get import get_cluster_info_assert_exists
from ...utils.infra import suggest_similar_flavor
from ...utils.register import DbaasOperation, Resource, get_request_handler, get_response_schema
from ...utils.types import RestoreHint
from ...utils.backups import get_cluster_id
from ..backups import get_cluster_backup_by_id, get_folder_by_backup_id


def _dump_hint_to_js(hint: RestoreHint, cluster_type: str) -> dict:
    schema = get_response_schema(cluster_type, Resource.BACKUP, DbaasOperation.RESTORE_HINTS)()
    res, errors = schema.dump(hint)
    if errors:
        raise RuntimeError(f"Restore hint dumped with errors: {errors}")
    return res


def _replace_decommissioning_flavors(hint: RestoreHint, cluster_type: str) -> RestoreHint:
    if hint.resources.resource_preset_id in config.get_decommissioning_flavors():
        hint.resources.resource_preset_id = suggest_similar_flavor(
            hint.resources.resource_preset_id, cluster_type, hint.resources.role.value
        )
    return hint


@API.resource('/mdb/<ctype:cluster_type>/<version>/console/restore-hints/<string:backup_id>')
class GetRestoreHintsV1(MethodView):
    """
    Get restore hints for given backup
    """

    @check_auth(
        folder_resolver=get_folder_by_backup_id,
        resource=Resource.BACKUP,
        operation=DbaasOperation.RESTORE_HINTS,
    )
    def get(self, cluster_type: str, backup_id: str, **_) -> dict:
        """
        Get restore hints by backup_id
        """
        try:
            handler = get_request_handler(cluster_type, Resource.BACKUP, DbaasOperation.RESTORE_HINTS)
        except UnsupportedHandlerError:
            raise DbaasNotImplementedError('Restore hints for {0} not implemented'.format(cluster_type))

        cluster_id = get_cluster_id(backup_id)
        info = get_cluster_info_assert_exists(cluster_id, cluster_type, include_deleted=True)
        backup = get_cluster_backup_by_id(info, backup_id)

        hint: RestoreHint = handler(info, backup)
        hint = _replace_decommissioning_flavors(hint, cluster_type)
        return _dump_hint_to_js(hint, cluster_type)

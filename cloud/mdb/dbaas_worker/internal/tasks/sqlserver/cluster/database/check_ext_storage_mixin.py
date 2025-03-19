"""
Mixin to validate external storage configuration and access before backup export/import
"""
from cloud.mdb.dbaas_worker.internal.exceptions import UserExposedException
from cloud.mdb.dbaas_worker.internal.tasks.sqlserver.utils import sqlserver_build_host_groups
from cloud.mdb.dbaas_worker.internal.providers.common import Change


class CheckExtStorageMixin:
    def check_ext_storage(self, operation, pillar):

        de_host_group, _ = sqlserver_build_host_groups(self.args['hosts'], self.config)
        host = sorted(list(de_host_group.hosts.keys()))[0]

        state_type = 'mdb_sqlserver.check_external_storage'
        shipment_id = self._call_salt_module_host(
            host,
            'operation=' + operation,
            self.args['hosts'][host]['environment'],
            state_type=state_type,
            pillar=pillar,
            title='storage-checker',
            rollback=Change.noop_rollback,
        )
        self.deploy_api.wait([shipment_id])

        result = self.deploy_api.get_result_for_shipment(shipment_id, host, state_type)  # Optional[dict]
        if result is None:
            self.logger.error("check_external_storage '%s' state not found for shipment %s", state_type, shipment_id)
            raise RuntimeError(operation + ' prohibited: no check_external_storage run found')

        if result.get('is_storage_ok') is None:
            self.logger.error(
                "check_external_storage '%s' state output for %s doesn't contain 'is_storage_ok': %r",
                state_type,
                shipment_id,
                result,
            )
            raise RuntimeError(operation + ' prohibited: no correct check_external_storage run found')

        if not result.get('is_storage_ok'):
            title = operation.capitalize().replace('-', ' ')
            stdout = result.get('info')
            raise UserExposedException(f'{title} error: {stdout}')

"""
MySQL Upgrade checker mixin.
"""

from typing import Any
from ....exceptions import UserExposedException
from ....providers.common import Change


class UpgradeCheckerMixin:
    """
    Mixin to run upgrade-checker operation before major/minor upgrade
    """

    def upgrade_checker_must_succeed(self: Any, cid: str, host: str):
        version = self.versions.get_cluster_version(cid, 'mysql')
        if version.major_version == '5.7':
            # mysql-shell upgrade checker cannot check minor mysql-5.7 upgrades
            return
        state_type = 'mdb_mysql.check_upgrade'
        shipment_id = self._call_salt_module_host(
            host,
            'target_version="{target_version}"'.format(target_version=version.minor_version),
            self.args['hosts'][host]['environment'],
            state_type=state_type,
            title='upgrade-checker',
            rollback=Change.noop_rollback,
        )
        self.deploy_api.wait([shipment_id])

        result = self.deploy_api.get_result_for_shipment(shipment_id, host, state_type)  # Optional[dict]
        # Expected format:
        # {
        #     'is_upgrade_safe': boolean,
        #     'comment': string,
        # }
        if result is None:
            self.logger.error("upgrade-checker '%s' state not found for shipment %s", state_type, shipment_id)
            raise RuntimeError('Upgrade prohibited: no upgrade-checker run found')

        if result.get('is_upgrade_safe') is None:
            self.logger.error(
                "upgrade-checker '%s' state output for %s doesn't contain 'is_upgrade_safe': %r",
                state_type,
                shipment_id,
                result,
            )
            raise RuntimeError('Upgrade prohibited: no correct upgrade-checker run found')

        if not result.get('is_upgrade_safe'):
            stdout = result.get('comment')
            raise UserExposedException(f'Upgrade prohibited by upgrade-checker. Fix following issues: {stdout}')

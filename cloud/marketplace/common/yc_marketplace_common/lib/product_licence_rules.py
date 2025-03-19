from cloud.marketplace.common.yc_marketplace_common.db.models import product_license_rules_table
from cloud.marketplace.common.yc_marketplace_common.models.product_license_rules import \
    ProductLicenseRule as ProductLicenseRuleScheme
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction
from yc_common import logging
from yc_common.clients.kikimr.client import _KikimrBaseConnection

log = logging.get_logger("yc_marketplace")


class ProductLicenseRules:
    @staticmethod
    @mkt_transaction()
    def get(version_id: str, *, tx: _KikimrBaseConnection) -> ProductLicenseRuleScheme:
        return tx.with_table_scope(product_license_rules_table).select_one(
            "SELECT * FROM $table WHERE id = ?", version_id,
            model=ProductLicenseRuleScheme)

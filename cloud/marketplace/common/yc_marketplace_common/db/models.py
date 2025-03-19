from cloud.marketplace.common.yc_marketplace_common.models.avatar import Avatar
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import Blueprint
from cloud.marketplace.common.yc_marketplace_common.models.build import Build
from cloud.marketplace.common.yc_marketplace_common.models.eula import Eula
from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.models.form import FormScheme
from cloud.marketplace.common.yc_marketplace_common.models.i18n import I18n
from cloud.marketplace.common.yc_marketplace_common.models.isv import Isv
from cloud.marketplace.common.yc_marketplace_common.models.ordering import Ordering
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProduct
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamily
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.partner_requests import PartnerRequest
from cloud.marketplace.common.yc_marketplace_common.models.product_license_rules import ProductLicenseRule
from cloud.marketplace.common.yc_marketplace_common.models.product_slug import ProductSlugScheme
from cloud.marketplace.common.yc_marketplace_common.models.product_to_sku_binding import ProductToSkuBindingScheme
from cloud.marketplace.common.yc_marketplace_common.models.publisher import Publisher
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProduct
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProduct
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraft
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.var import Var
from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTable
from yc_common.clients.kikimr import KikimrTableSpec
from yc_common.clients.kikimr import get_kikimr_client
from yc_marketplace_migrations.migration import MigrationTable

db_name = "marketplace"
DB_LIST_BY_NAME = {}


def marketplace_db(**kwargs):
    return get_kikimr_client(db_name, **kwargs)


def _make_table(*args, **kwargs) -> KikimrTable:
    global DB_LIST_BY_NAME

    table = KikimrTable(db_name, *args, **kwargs)
    DB_LIST_BY_NAME[table.name] = table

    return table


publishers_table = _make_table(*(Publisher.data_model.serialize_to_common()[0]))
isv_table = _make_table(*(Isv.data_model.serialize_to_common()[0]))
var_table = _make_table(*(Var.data_model.serialize_to_common()[0]))

partner_requests_table = _make_table(*(PartnerRequest.data_model.serialize_to_common()[0]))

categories_table = _make_table(*(Category.data_model.serialize_to_common()[0]))

ordering_table = _make_table(*(Ordering.data_model.serialize_to_common()[0]))

avatars_table = _make_table(*(Avatar.data_model.serialize_to_common()[0]))

eulas_table = _make_table(*(Eula.data_model.serialize_to_common()[0]))

os_products_table = _make_table(*(OsProduct.data_model.serialize_to_common()[0]))

os_product_families_table = _make_table(*(OsProductFamily.data_model.serialize_to_common()[0]))

os_product_family_versions_table = _make_table(*(OsProductFamilyVersion.data_model.serialize_to_common()[0]))

product_license_rules_table = _make_table(*(ProductLicenseRule.data_model.serialize_to_common()[0]))

saas_products_table = _make_table(*(SaasProduct.data_model.serialize_to_common()[0]))

simple_products_table = _make_table(*(SimpleProduct.data_model.serialize_to_common()[0]))

product_slugs_table = _make_table(*(ProductSlugScheme.data_model.serialize_to_common()[0]))

sku_draft_table = _make_table(*(SkuDraft.data_model.serialize_to_common()[0]))

product_to_sku_binding_table = _make_table(*(ProductToSkuBindingScheme.data_model.serialize_to_common()[0]))

tasks_table = _make_table(*(Task.data_model.serialize_to_common()[0]))  # TODO multiple tables

forms_table = _make_table(*(FormScheme.data_model.serialize_to_common()[0]))

blueprints_table = _make_table(*(Blueprint.data_model.serialize_to_common()[0]))

builds_table = _make_table(*(Build.data_model.serialize_to_common()[0]))

migrations_table = _make_table(MigrationTable().name, KikimrTableSpec(
    columns={
        col: getattr(KikimrDataType, MigrationTable.columns[col]) for col in MigrationTable.columns
    },
    primary_keys=MigrationTable.primary_keys,
))

i18n_table = _make_table(*(I18n.data_model.serialize_to_common()[0]))

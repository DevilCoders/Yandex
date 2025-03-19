import time
from typing import Dict
from typing import List

from yc_common import config
from yc_common import logging
from yc_common.clients.compute import ComputeClient
from yc_common.clients.kikimr.client import TransactionMode
from yc_common.clients.kikimr.client import _KikimrBaseConnection
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.clients.models import images
from yc_common.misc import drop_none
from yc_common.misc import timestamp
from yc_common.paging import page_handler
from yc_common.validation import ResourceIdType

from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import os_product_families_table
from cloud.marketplace.common.yc_marketplace_common.lib.i18n import I18n
from cloud.marketplace.common.yc_marketplace_common.models.deprecation import Deprecation
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import \
    OsProductFamily as OsProductFamilyScheme
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyDeprecationRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyList
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyMetadata
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyOperation
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyResponse
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyUpdateRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersion as OsProductFamilyVersionScheme
from cloud.marketplace.common.yc_marketplace_common.models.resource_spec import ResourceSpec
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidComputeImageStatus
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidProductFamilyDeprecationError
from cloud.marketplace.common.yc_marketplace_common.utils.errors import OsProductFamilyIdError
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction
from cloud.marketplace.common.yc_marketplace_common.utils.errors import InvalidRelatedProductError

log = logging.get_logger("yc_marketplace")


class OsProductFamily:
    @staticmethod
    @mkt_transaction()
    def get(family_id: str, *, tx: _KikimrBaseConnection) -> OsProductFamilyScheme:
        return tx.with_table_scope(os_product_families_table).select_one(
            "SELECT " + OsProductFamilyScheme.db_fields() + " FROM $table WHERE id = ?", family_id,
            model=OsProductFamilyScheme)

    @staticmethod
    @mkt_transaction()
    def get_full_view(family_id: str, *, tx: _KikimrBaseConnection) -> OsProductFamilyResponse:
        family = lib.OsProductFamily.get(family_id, tx=tx)
        if family is None:
            log.error("Family is broken", family_id)
            raise OsProductFamilyIdError()

        version = lib.OsProductFamilyVersion.get_by_filter({
            "os_product_family_id = ?": family_id,
            "status = ?": OsProductFamilyVersionScheme.Status.ACTIVE,
        }, tx=tx)

        if version is not None:
            version = version.to_public()

        response = family.to_public()
        response.active_version = version

        return response

    @staticmethod
    @mkt_transaction()
    def rpc_get_full(family_id: str, *, tx: _KikimrBaseConnection) -> OsProductFamilyResponse:
        return OsProductFamily.get_full_view(family_id, tx=tx)

    @staticmethod
    @mkt_transaction()
    def get_by_product_id(product_id: ResourceIdType, *, tx) -> List[OsProductFamilyResponse]:
        families = tx.with_table_scope(os_product_families_table).select(
            "SELECT " + OsProductFamilyScheme.db_fields() + " FROM $table WHERE os_product_id = ?",
            product_id, model=OsProductFamilyScheme)

        resp = []
        for family in families:
            version = lib.OsProductFamilyVersion.get_by_filter({
                "os_product_family_id = ?": family.id,
                "status = ?": OsProductFamilyVersionScheme.Status.ACTIVE,
            }, tx=tx)
            if version is None:
                continue

            public_family = family.to_public()
            public_family.active_version = version.to_public()
            resp.append(public_family)

        return resp

    @staticmethod
    def _prepare_list_sql_conditions(cursor, filter_query, limit, order_by,
                                     *, billing_account_id=None, public=False) -> (List[str], str):
        filter_query, filter_args = parse_filter(filter_query, OsProductFamilyScheme)

        if billing_account_id:
            filter_query += ["billing_account_id = ?"]
            filter_args += [billing_account_id]

        if public:
            filter_query += ["?"]
            filter_args += [SqlIn("status", OsProductFamilyScheme.Status.PUBLIC)]

        if filter_query is not None:
            filter_query = " AND ".join(filter_query)

        where_query, where_args = page_query_args(
            cursor,
            limit,
            id="id",
            filter_query=filter_query,
            filter_args=filter_args,
            order_by=order_by,
        )
        return where_args, where_query

    @staticmethod
    @mkt_transaction()
    def list(where_query, where_args, limit, *, tx) -> OsProductFamilyList:
        families = tx.with_table_scope(os_product_families_table).select(
            "SELECT " + OsProductFamilyScheme.db_fields() + " " +
            "FROM $table " + where_query,
            *where_args, model=OsProductFamilyScheme)

        resp = OsProductFamilyList()
        for family in families:
            resp.os_product_families.append(family.to_public())

        if limit is not None and limit == len(resp.os_product_families):
            resp.next_page_token = resp.os_product_families[-1].id

        return resp

    @staticmethod
    @mkt_transaction()
    @page_handler(items="os_product_families")
    def rpc_list_public(cursor, limit, order_by=None, filter=None, tx=None) -> OsProductFamilyList:

        where_args, where_query = OsProductFamily._prepare_list_sql_conditions(cursor, filter, limit, order_by,
                                                                               public=True)

        return OsProductFamily.list(where_query, where_args, limit, tx=tx)

    @staticmethod
    @mkt_transaction()
    @page_handler(items="os_product_families")
    def rpc_list(cursor, limit, billing_account_id, order_by=None, filter=None, tx=None) -> OsProductFamilyList:
        where_args, where_query = OsProductFamily._prepare_list_sql_conditions(cursor, filter, limit, order_by,
                                                                               billing_account_id=billing_account_id)

        return OsProductFamily.list(where_query, where_args, limit, tx=tx)

    @staticmethod
    @mkt_transaction()
    def rpc_get_by_family_ids(family_ids: List[str], *, tx) -> Dict[str, OsProductFamilyResponse]:
        families_iter = tx.with_table_scope(os_product_families_table).select(
            "SELECT %s FROM $table WHERE ?" % OsProductFamilyScheme.db_fields(),
            SqlIn("id", family_ids), model=OsProductFamilyScheme)  # because query scope in global =(

        versions_iter = lib.OsProductFamilyVersion.get_active_by_family_ids(family_ids, tx=tx)
        result = {}
        versions_by_family = {version.os_product_family_id: version.to_public() for version in versions_iter}
        for family in families_iter:
            result[family.id] = family.to_public()
            version = versions_by_family.get(family.id)
            if version is None:
                continue

            result[family.id].active_version = version
        return result

    @staticmethod
    @mkt_transaction(tx_mode=TransactionMode.ONLINE_READ_ONLY_CONSISTENT)
    def rpc_get_by_product_ids(os_product_ids: List[str], *, tx) -> Dict[str, List[OsProductFamilyResponse]]:
        start_method_time = time.monotonic()

        start_query_time = time.monotonic()
        # table_tx = tx.with_table_scope(os_product_families_table)
        # query = """
        #     DECLARE $ydb_ids as "List<Struct<id: Utf8>>";
        #
        #     SELECT {}
        #     FROM as_table($ydb_ids) as ids
        #     INNER JOIN $table as product_family
        #     ON product_family.os_product_id = ids.id
        #     WHERE status="{}"
        # """.format(OsProductFamilyScheme.db_fields("product_family"), OsProductFamilyScheme.Status.ACTIVE)
        # p_query = table_tx.prepare_query(query)
        # families_iter = table_tx.select(p_query, {"$ydb_ids": [{"id": id} for id in os_product_ids]},
        #                                 model=OsProductFamilyScheme)

        families_iter = tx.with_table_scope(os_product_families_table) \
            .select("SELECT " + OsProductFamilyScheme.db_fields() + " FROM $table WHERE ? AND status = ?",
                    SqlIn("os_product_id", os_product_ids), OsProductFamilyScheme.Status.ACTIVE,
                    model=OsProductFamilyScheme)

        log.debug("Query rpc_get_by_product_ids_1 time: %s ms" % (time.monotonic() - start_query_time))

        versions_iter = lib.OsProductFamilyVersion.get_active_by_family_ids([family.id for family in families_iter],
                                                                            tx=tx)
        versions_by_family = {version.os_product_family_id: version.to_public() for version in versions_iter}
        result = {}
        for family in families_iter:
            if family.os_product_id not in result:
                result[family.os_product_id] = []
            public_family = family.to_public()
            public_family.active_version = versions_by_family.get(family.id)
            result[family.os_product_id] += [public_family]

        log.debug("Method rpc_get_by_product_ids time: %s ms" % (time.monotonic() - start_method_time))
        return result

    @staticmethod
    @mkt_transaction()
    def rpc_create(request: OsProductFamilyCreateRequest, *, tx) -> OsProductFamilyOperation:
        lib.Publisher.check_state(request.billing_account_id)
        OsProductFamily.validate_image(request.image_id)

        meta = request.meta

        family = OsProductFamilyScheme.new(
            billing_account_id=request.billing_account_id,
            os_product_id=request.os_product_id,
            name=request.name,
            description=request.description,
            meta={},
        )

        if meta is not None:
            for key in meta:
                meta[key] = I18n.set("os_product_family.{}.meta.{}".format(family.id, key), meta[key])

        family.meta = meta

        family.name = (family.name or "").format(id=family.id)
        family.description = (family.description or "").format(id=family.id)

        tx.with_table_scope(os_product_families_table).insert_object("INSERT INTO $table", family)

        OsProductFamily.validate_related_products(request.related_products, request.billing_account_id)

        return lib.OsProductFamilyVersion.create(
            billing_account_id=request.billing_account_id,
            os_product_family_id=family.id,
            family_slug=request.slug,
            pricing_options=request.pricing_options,
            resource_spec=ResourceSpec.new_from_model(request.resource_spec),
            image_id=request.image_id,
            logo_id=request.logo_id,
            skus=request.skus,
            related_products=request.related_products,
            license_rules=request.license_rules,
            task_group_id=generate_id(),
            tx=tx,
        )

    @staticmethod
    @mkt_transaction()
    def rpc_update(request: OsProductFamilyUpdateRequest, *, tx) -> OsProductFamilyOperation:
        family = OsProductFamily.get_full_view(request.os_product_family_id)
        lib.Publisher.check_state(family.billing_account_id)

        OsProductFamily.validate_related_products(request.related_products, family.billing_account_id)
        OsProductFamily.validate_image(request.image_id)

        version_update = drop_none({
            "image_id": request.image_id,
            "resource_spec": None if request.resource_spec is None else ResourceSpec.new_from_model(
                request.resource_spec),  # noqa
            "pricing_options": request.pricing_options,
            "skus": request.skus,
            "logo_id": request.logo_id,
            "related_products": request.related_products,
            "license_rules": request.license_rules,
        })

        version_op = None
        task_group_id = generate_id()
        if version_update:
            active = family.active_version
            log.info("active %s", active)
            base_version = {}
            if active:
                rules = lib.ProductLicenseRules.get(active.id)
                base_version = {
                    "resource_spec": active.resource_spec,
                    "pricing_options": active.pricing_options,
                    "skus": active.skus,
                    "logo_id": active.logo_id,
                    "related_products": active.related_products,
                    "license_rules": rules
                }

            version_update = {**base_version, **version_update}
            log.info("Version_update: %s", version_update)

            version_op = lib.OsProductFamilyVersion.create(
                billing_account_id=family.billing_account_id,
                os_product_family_id=request.os_product_family_id,
                family_slug=request.slug,
                pricing_options=version_update["pricing_options"],
                resource_spec=version_update["resource_spec"],
                image_id=request.image_id,
                logo_id=version_update.get("logo_id"),
                skus=version_update.get("skus"),
                related_products=version_update.get("related_products"),
                task_group_id=task_group_id,
                license_rules=version_update.get("license_rules"),
                tx=tx,
            )

        family_update = drop_none({
            "labels": request.labels,
            "name": request.name,
            "description": request.description,
            "meta": request.meta,
        })

        depends = [version_op.id] if version_op is not None else []

        return OsProductFamily.task_update(
            task_group_id=task_group_id,
            family_id=request.os_product_family_id,
            family_update=family_update,
            depends=depends,
            tx=tx,
        )

    @staticmethod
    def validate_image(image_id):
        """Check if image state is Ready"""
        if image_id and not config.get_value("dev", False):
            compute_public_client = ComputeClient(
                url=config.get_value("endpoints.compute.url"),
                iam_token=metadata_token.get_instance_metadata_token(),
            )
            image = compute_public_client.get_image(image_id)

            if image.status != images.Image.Status.READY:
                raise InvalidComputeImageStatus()

    @staticmethod
    def validate_related_products(related_products, billing_account_id):
        if related_products:
            for p in related_products:
                related_product = lib.SaasProduct.get(product_id=p.product_id)
                if related_product.billing_account_id != billing_account_id:
                    raise InvalidRelatedProductError()

    @staticmethod
    @mkt_transaction()
    def update(family_id: str, update: dict, *, tx) -> None:
        if not update:
            return

        if "meta" in update:
            for key in update["meta"]:
                update["meta"][key] = I18n.set("os_product_family.{}.meta.{}".format(family_id, key),
                                               update["meta"][key])

        update["name"] = update.get("name", "").format(id=family_id)
        update["description"] = update.get("description", "").format(id=family_id)

        update["updated_at"] = timestamp()
        tx.with_table_scope(os_product_families_table) \
            .update_object("UPDATE $table $set WHERE id = ?", update, family_id)

        if "status" in update:
            family = OsProductFamily.get(family_id)
            other_active = tx.with_table_scope(os_product_families_table) \
                .select_one("SELECT " + OsProductFamilyScheme.db_fields() + " FROM $table " +
                            "WHERE status = ? AND id  != ? AND os_product_id = ? ORDER BY updated_at LIMIT 1",
                            OsProductFamilyScheme.Status.ACTIVE, family.id, family.os_product_id,
                            model=OsProductFamilyScheme)

            lib.OsProduct.on_change_family(family.os_product_id, family, other_active, tx=tx)

    @staticmethod
    @mkt_transaction()
    def rpc_deprecate(request: OsProductFamilyDeprecationRequest, *, tx) -> OsProductFamilyOperation:
        family_id = request.os_product_family_id

        family = OsProductFamily.get_full_view(family_id)
        current_deprecation_status = None
        if family.deprecation:
            current_deprecation_status = family.deprecation.status

        valid_transitions = Deprecation.Status.Transitions.get(current_deprecation_status)
        if valid_transitions is None or request.deprecation.status not in valid_transitions:
            raise InvalidProductFamilyDeprecationError()

        other_active = tx.with_table_scope(os_product_families_table) \
            .select_one("SELECT " + OsProductFamilyScheme.db_fields() + " FROM $table " +
                        "WHERE status = ? AND id != ? AND os_product_id = ?  ORDER BY updated_at LIMIT 1",
                        OsProductFamilyScheme.Status.ACTIVE, family.id, family.os_product_id,
                        model=OsProductFamilyScheme)

        ts = timestamp()

        if request.deprecation.status == Deprecation.Status.OBSOLETE:
            lib.OsProductFamilyVersion.deprecate_from_family(family_id, ts, tx=tx)

            family.status = OsProductFamilyScheme.Status.DEPRECATED
            if family.active_version:
                family.active_version.status = OsProductFamilyVersionScheme.Status.DEPRECATED
                family.active_version.updated_at = ts

        tx.with_table_scope(os_product_families_table).update_object(
            "UPDATE $table $set WHERE id = ?",
            {
                "deprecation": request.deprecation.to_kikimr(),
                "updated_at": ts,
                "status": family.status,
            },
            family_id,
        )
        family.deprecation = request.deprecation
        family.updated_at = ts

        lib.OsProduct.on_change_family(family.os_product_id, family, other_active, tx=tx)

        op = OsProductFamilyOperation({
            "id": family.id,
            "description": "os_product_family_deprecate",
            "created_at": timestamp(),
            "done": True,
            "metadata": OsProductFamilyMetadata({"os_product_family_id": family_id}).to_primitive(),
            "response": family.to_api(True),
        })

        return lib.TaskUtils.fake(op, "fake_os_product_family_deprecate", tx=tx)

    @staticmethod
    @mkt_transaction()
    def on_change_version_status(version_dict: dict, *, tx: _KikimrBaseConnection, force=False):
        version = version_dict["version"]
        family = OsProductFamily.get(version.os_product_family_id)

        if version.status == OsProductFamilyVersionScheme.Status.ACTIVE:
            if family.status != OsProductFamilyScheme.Status.ACTIVE:
                OsProductFamily._rpc_set_status(family, OsProductFamilyScheme.Status.ACTIVE, tx=tx)
        else:
            if version_dict["previous_active"] is None:
                OsProductFamily._rpc_set_status(family, OsProductFamilyScheme.Status.from_version(version), tx=tx)

    @staticmethod
    @mkt_transaction()
    def _rpc_set_status(family, status: str, *, tx: _KikimrBaseConnection):
        family.status = status
        other_active = tx.with_table_scope(os_product_families_table) \
            .select_one("SELECT " + OsProductFamilyScheme.db_fields() + " FROM $table " +
                        "WHERE status = ? AND id  != ? AND os_product_id = ? ORDER BY updated_at LIMIT 1",
                        OsProductFamilyScheme.Status.ACTIVE, family.id, family.os_product_id,
                        model=OsProductFamilyScheme)

        tx.with_table_scope(os_product_families_table).update_object(
            "UPDATE $table $set WHERE id = ?",
            {
                "status": status,
                "updated_at": timestamp(),
            },
            family.id,
        )

        lib.OsProduct.on_change_family(family.os_product_id, family, other_active)

    @staticmethod
    @mkt_transaction()
    def task_update(*, task_group_id: str, family_id: str, family_update: dict, depends: List[ResourceIdType],
                    tx: _KikimrBaseConnection):
        return lib.TaskUtils.create(
            "update_product_family",
            params={
                "os_product_family_id": family_id,
                "update": family_update,
            },
            group_id=task_group_id,
            depends=depends,
            metadata=OsProductFamilyMetadata({
                "os_product_family_id": family_id,
            }).to_primitive(),
            tx=tx,
        )

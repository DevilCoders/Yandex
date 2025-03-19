import os
import time
from typing import List
from typing import Optional

from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import os_product_family_versions_table
from cloud.marketplace.common.yc_marketplace_common.db.models import product_license_rules_table
from cloud.marketplace.common.yc_marketplace_common.lib import Avatar
from cloud.marketplace.common.yc_marketplace_common.lib.os_product_family import OsProductFamily
from cloud.marketplace.common.yc_marketplace_common.lib.task import TaskUtils
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyOperation
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersion as OsProductFamilyVersionScheme  # noqa
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersionList
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionMetadata
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionOperation
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionResponse
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import Sku
from cloud.marketplace.common.yc_marketplace_common.models.product_license_rules import \
    ProductLicenseRule  # noqa
from cloud.marketplace.common.yc_marketplace_common.models.product_reference import ProductReference
from cloud.marketplace.common.yc_marketplace_common.models.task import BindSkusToVersionParams
from cloud.marketplace.common.yc_marketplace_common.models.task import DeleteImagePoolParams
from cloud.marketplace.common.yc_marketplace_common.models.task import FinalizeImageParams
from cloud.marketplace.common.yc_marketplace_common.models.task import OsProductFamilyVersionCreateParams
from cloud.marketplace.common.yc_marketplace_common.models.task import StartCloneImageParams
from cloud.marketplace.common.yc_marketplace_common.models.task import StartPublishVersionParams
from cloud.marketplace.common.yc_marketplace_common.models.task import UpdateImagePoolSizeParams
from cloud.marketplace.common.yc_marketplace_common.utils.errors import OsProductFamilyVersionIdError
from cloud.marketplace.common.yc_marketplace_common.utils.errors import OsProductFamilyVersionStatusUpdateError
from cloud.marketplace.common.yc_marketplace_common.utils.errors import OsProductFamilyVersionUpdateError
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_order_by
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction
from yc_common import config
from yc_common import logging
from yc_common.clients.kikimr import TransactionMode
from yc_common.clients.kikimr.client import _KikimrTxConnection
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.clients.kikimr.sql import SqlNotIn
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.misc import timestamp
from yc_common.paging import page_handler
from yc_common.validation import ResourceIdType

log = logging.get_logger("yc_marketplace")

UPDATEABLE_FIELDS = {
    "image_id",  # queue task update this field when move image through our publishing pipeline.
    "resource_spec",
    "status",
}

_V_Status = OsProductFamilyVersionScheme.Status


class _Flow:
    STILL = "still"
    MOVE = "move"
    IMPOSSIBLE = "impossible"
    BROKEN = "broken"

    _flow_table = {
        _V_Status.PENDING: {None, _V_Status.ERROR},
        _V_Status.REVIEW: {_V_Status.PENDING, _V_Status.ERROR, _V_Status.REJECTED},
        _V_Status.ACTIVATING: {_V_Status.REVIEW},
        _V_Status.ACTIVE: {_V_Status.ACTIVATING},
        _V_Status.DEPRECATED: {_V_Status.ACTIVE},
        _V_Status.REJECTED: _V_Status.ALL - {_V_Status.ERROR},
        _V_Status.ERROR: _V_Status.ALL,
    }

    @staticmethod
    def _check_flow(previous, follow):
        if previous == follow:
            return _Flow.STILL

        if follow not in _Flow._flow_table:
            return _Flow.BROKEN

        return _Flow.MOVE if previous in _Flow._flow_table[follow] else _Flow.IMPOSSIBLE


class OsProductFamilyVersion:
    @staticmethod
    @mkt_transaction()
    def get(id, *, tx) -> OsProductFamilyVersionScheme:
        version = tx.with_table_scope(os_product_family_versions_table).select_one(
            "SELECT " + OsProductFamilyVersionScheme.db_fields() + " " +
            "FROM $table "
            "WHERE id = ?", id,
            model=OsProductFamilyVersionScheme)

        return version if version is not None else None

    @staticmethod
    def rpc_get(version_id) -> OsProductFamilyVersionResponse:
        version = OsProductFamilyVersion.get(version_id)

        if version is None:
            raise OsProductFamilyVersionIdError()

        return version.to_public()

    @staticmethod
    @mkt_transaction()
    def get_by_filter(filter_dict, *, tx) -> OsProductFamilyVersionResponse:

        where_query_parts = []
        where_args = []
        for column in filter_dict:
            where_query_parts.append(column)
            where_args.append(filter_dict[column])

        where_query = ""
        if len(where_query_parts) > 0:
            where_query = "WHERE %s " % " AND ".join(where_query_parts)

        return tx.with_table_scope(os_product_family_versions_table).select_one(
            "SELECT " + OsProductFamilyVersionScheme.db_fields() + " " +
            "FROM $table " +
            where_query +
            "LIMIT 1 ", *where_args,
            model=OsProductFamilyVersionScheme)

    @staticmethod
    @mkt_transaction()
    def get_batch(ids, *, tx) -> List[OsProductFamilyVersionResponse]:
        if not ids:
            return []

        return tx.with_table_scope(os_product_family_versions_table).select(
            "SELECT " + OsProductFamilyVersionScheme.db_fields() + " " +
            "FROM $table " +
            "WHERE ?", SqlIn('id', ids),
            model=OsProductFamilyVersionScheme)

    @staticmethod
    @mkt_transaction(tx_mode=TransactionMode.ONLINE_READ_ONLY_CONSISTENT)
    def get_active_by_family_ids(family_ids, *, tx: _KikimrTxConnection):
        start_time = time.monotonic()

        ts_transaction = tx.with_table_scope(os_product_family_versions_table)
        # query = """
        #     DECLARE $ydb_ids as "List<Struct<id: Utf8>>";
        #     SELECT {}
        #     FROM as_table($ydb_ids) as ids
        #     INNER JOIN $table as os_product_family_version
        #     ON os_product_family_version.os_product_family_id = ids.id
        #     WHERE status="{}";
        # """.format(OsProductFamilyVersionScheme.db_fields("os_product_family_version"), _V_Status.ACTIVE)
        # p_query = ts_transaction.prepare_query(query)
        # result = ts_transaction.select(p_query, {"$ydb_ids": [{"id": id} for id in family_ids]},
        #                                model=OsProductFamilyVersionScheme)
        result = ts_transaction.select("SELECT " + OsProductFamilyVersionScheme.db_fields() +
                                       " FROM $table WHERE ? AND status = ?", SqlIn("os_product_family_id", family_ids),
                                       _V_Status.ACTIVE, model=OsProductFamilyVersionScheme)

        log.debug("Method get_active_by_family_ids time: %s ms" % (time.monotonic() - start_time))
        return result

    @staticmethod
    @mkt_transaction()
    @page_handler(items="os_product_family_versions")
    def rpc_public_list(cursor, limit, filter, billing_account_id, order_by=None, *, tx) -> OsProductFamilyVersionList:
        filter_query, filter_args = parse_filter(filter, OsProductFamilyVersionScheme)
        mapping = {
            "id": "id",
            "createdAt": "created_at",
            "updatedAt": "updated_at",
            "publishedAt": "published_at",
            "status": "status",
        }

        order_by = parse_order_by(order_by, mapping, "id")

        filter_query = ["billing_account_id = ?"] + filter_query
        filter_args = [billing_account_id] + filter_args
        return OsProductFamilyVersion._list(cursor, filter_args, filter_query, limit, order_by, tx)

    @staticmethod
    @mkt_transaction()
    @page_handler(items="os_product_family_versions")
    def rpc_list(cursor, limit, filter, order_by=None, *, tx) -> OsProductFamilyVersionList:
        filter_query, filter_args = parse_filter(filter, OsProductFamilyVersionScheme)
        mapping = {
            "id": "id",
            "createdAt": "created_at",
            "updatedAt": "updated_at",
            "publishedAt": "published_at",
            "status": "status",
        }

        order_by = parse_order_by(order_by, mapping, "id")

        return OsProductFamilyVersion._list(cursor, filter_args, filter_query, limit, order_by, tx)

    @staticmethod
    def _list(cursor, filter_args, filter_query, limit, order_by, tx):
        where_query, where_args = page_query_args(
            cursor,
            limit,
            id="id",
            filter_query=" AND ".join(filter_query),
            filter_args=filter_args,
            order_by=order_by,
        )

        iterator = tx.with_table_scope(os_product_family_versions_table).select(
            "SELECT " + OsProductFamilyVersionScheme.db_fields() + " " +
            "FROM $table " + where_query,
            *where_args, model=OsProductFamilyVersionScheme)

        response = OsProductFamilyVersionList()
        for n, v in enumerate(iterator):
            if limit is not None and n == limit - 1:
                response.next_page_token = v.id
            response.os_product_family_versions.append(v.to_public())

        return response

    @staticmethod
    @mkt_transaction()
    def create(billing_account_id, os_product_family_id, family_slug, pricing_options, resource_spec,
               logo_id, image_id=None, skus: list = None, related_products=None, task_group_id=None,
               license_rules=None, *, tx=None) -> Optional[OsProductFamilyOperation]:
        if skus is not None:
            skus = [Sku(s) for s in skus]

        if related_products is not None:
            related_products = [ProductReference(p) for p in related_products]

        version = OsProductFamilyVersionScheme.new(
            billing_account_id=billing_account_id,
            os_product_family_id=os_product_family_id,
            image_id=image_id,
            pricing_options=pricing_options,
            resource_spec=resource_spec,
            status=_V_Status.PENDING,
            logo_id=logo_id,
            skus=skus,
            related_products=related_products,
        )

        if version.logo_id is not None:
            Avatar.capture_image(version.logo_id, version.id, tx=tx)

        tx.with_table_scope(os_product_family_versions_table).insert_object("INSERT INTO $table", version)

        if license_rules:
            rules = ProductLicenseRule({
                "id": version.id,
                "rules": [ProductLicenseRule.Rule({**r}) for r in license_rules]
            })
            tx.with_table_scope(product_license_rules_table).insert_object("INSERT INTO $table", rules)

        if image_id is not None:
            clone_task = OsProductFamilyVersion.task_clone(
                task_group_id=task_group_id,
                version_id=version.id,
                family_slug=family_slug,
                image_id=image_id,
                resource_spec=resource_spec,
                tx=tx,
            )
            depends = [clone_task.id]

            return OsProductFamilyVersion.task_finalize(
                task_group_id=task_group_id,
                version_id=version.id,
                family_id=version.os_product_family_id,
                target_status=OsProductFamilyVersionScheme.Status.REVIEW,
                depends=depends,
                tx=tx,
            )

        return None

    @staticmethod
    @mkt_transaction()
    def update(version_id, update_dict, *, tx, propagate=False, force=False):
        version = OsProductFamilyVersion.get(version_id)
        propagate_to_family = False

        if version is None:
            raise OsProductFamilyVersionIdError()

        previous_active = OsProductFamilyVersion.get_by_filter({
            "id != ?": version.id,
            "status = ?": OsProductFamilyVersionScheme.Status.ACTIVE,
            "os_product_family_id = ?": version.os_product_family_id,
        })  # No use current transaction for find alternative primary version

        if "status" in update_dict:
            check = _Flow._check_flow(version.status, update_dict["status"])

            if check == _Flow.IMPOSSIBLE or check == _Flow.BROKEN:
                log.warn("Try to brake version!", version.status, update_dict["status"], check, force)
                if not force:
                    raise OsProductFamilyVersionStatusUpdateError()

                check = _Flow.MOVE

            propagate_to_family = check == _Flow.MOVE

        props = update_dict.keys()
        if not set(props).issubset(UPDATEABLE_FIELDS) and not force:
            log.warn("Try to update immutable fields in version!", update_dict)
            raise OsProductFamilyVersionUpdateError()

        update_dict["updated_at"] = timestamp()
        if update_dict.get("status") == _V_Status.ACTIVE and version.status != _V_Status.ACTIVE:
            update_dict["published_at"] = timestamp()
            if previous_active:
                previous_active.status = _V_Status.DEPRECATED
                tx.with_table_scope(os_product_family_versions_table).insert_object("UPSERT INTO $table",
                                                                                    previous_active)

        for prop in props:
            if hasattr(version, prop):
                setattr(version, prop, update_dict[prop])
            else:
                log.warn("Try to update nonexistent fields in version!", prop, update_dict)
                raise OsProductFamilyVersionUpdateError()

        tx.with_table_scope(os_product_family_versions_table).insert_object("UPSERT INTO $table", version)
        version_dict = {
            "propagate_to_family": propagate_to_family,
            "version": version,
            "previous_active": previous_active,
        }

        if propagate:
            OsProductFamily.on_change_version_status(version_dict, tx=tx)

        return version_dict

    @staticmethod
    @mkt_transaction()
    def rpc_force_update(version_id, update_dict, *, tx) -> OsProductFamilyVersionOperation:
        version_dict = OsProductFamilyVersion.update(version_id, update_dict, tx=tx, force=True)

        if version_dict["propagate_to_family"]:
            OsProductFamily.on_change_version_status(version_dict, tx=tx, force=True)

        op = OsProductFamilyVersionOperation({
            "id": version_id,
            "description": "os_product_family_version_update",
            "created_at": timestamp(),
            "done": True,
            "metadata": OsProductFamilyVersionMetadata({
                "os_product_family_id": version_dict["version"].os_product_family_id,
                "os_product_family_version_id": version_id,
            }).to_primitive(),
            "response": version_dict["version"].to_public().to_api(True),
        })

        return TaskUtils.fake(op, "fake_os_product_family_version_update", tx=tx)

    @staticmethod
    @mkt_transaction()
    def rpc_check_all_free(version_ids: List[ResourceIdType], *, tx) -> bool:
        result = tx.with_table_scope(os_product_family_versions_table) \
            .select("SELECT pricing_options "
                    "FROM $table WHERE ? ",
                    SqlIn("id", version_ids))
        pricing_options = [x["pricing_options"] for x in result]

        if len(pricing_options) != len(version_ids):
            raise OsProductFamilyVersionIdError()

        return all(po == OsProductFamilyVersionScheme.PricingOptions.FREE for po in pricing_options)

    @staticmethod
    @mkt_transaction()
    def rpc_publish(version_id: ResourceIdType, image_pool_size: int = 1, *, tx) -> OsProductFamilyVersionOperation:
        target_folder_id = os.getenv("MARKETPLACE_PUBLIC_IMG_FOLDER_ID")
        if not target_folder_id:
            raise EnvironmentError("MARKETPLACE_PUBLIC_IMG_FOLDER_ID is not set")

        version_dict = OsProductFamilyVersion.update(version_id, {
            "status": OsProductFamilyVersionScheme.Status.ACTIVATING,
        }, tx=tx)
        if version_dict["propagate_to_family"]:
            OsProductFamily.on_change_version_status(version_dict, tx=tx)
        task_group_id = generate_id()
        version = version_dict["version"]

        publish_task = OsProductFamilyVersion.task_publish(
            task_group_id=task_group_id,
            version_id=version_id,
            target_folder_id=target_folder_id,
            tx=tx,
        )

        bind_skus_task = OsProductFamilyVersion.task_bind_sku(
            task_group_id=task_group_id,
            version_id=version_id,
            tx=tx,
        )

        fin_task = OsProductFamilyVersion.task_finalize(
            task_group_id=task_group_id,
            version_id=version_id,
            family_id=version.os_product_family_id,
            target_status=OsProductFamilyVersionScheme.Status.ACTIVE,
            depends=[publish_task.id, bind_skus_task.id],
            tx=tx,
        )

        depends = []
        if version.logo_id is not None:
            publish_logo = Avatar.task_publish(
                version.logo_id,
                str(version_id),
                config.get_value("endpoints.s3.versions_bucket"),
                group_id=task_group_id,
                tx=tx,
            )
            depends = [publish_logo.id]

        update_pool_size = OsProductFamilyVersion.task_update_image_pool_size(
            pool_size=image_pool_size,
            task_group_id=task_group_id,
            depends=[fin_task.id],
            tx=tx,
        )

        finalize_image_pool_size = OsProductFamilyVersion.task_finalize_image_pool_size(
            task_group_id=task_group_id,
            depends=[update_pool_size.id],
            tx=tx,
        )

        return OsProductFamilyVersion.task_create(
            task_group_id=task_group_id,
            version_id=version_id,
            family_id=version.os_product_family_id,
            depends=depends + [finalize_image_pool_size.id],
            tx=tx,
        )

    @staticmethod
    @mkt_transaction()
    def deprecate_from_family(family_id, ts=timestamp(), *, tx):
        res = tx.with_table_scope(os_product_family_versions_table) \
            .select("SELECT image_id FROM $table WHERE os_product_family_id = ? AND ?",
                    family_id, SqlNotIn("status", _V_Status.FINAL))
        tx.with_table_scope(os_product_family_versions_table) \
            .update_object("UPDATE $table $set WHERE os_product_family_id = ? AND ?",
                           {
                               "status": _V_Status.DEPRECATED,
                               "updated_at": ts,
                           }, family_id, SqlNotIn("status", _V_Status.FINAL))
        group_id = generate_id()
        for row in res:
            OsProductFamilyVersion.task_delete_disk_pooling(
                task_group_id=group_id,
                image_id=row["image_id"],
                tx=tx,
            )

    @staticmethod
    @mkt_transaction()
    def rpc_get_batch_logo_uri(version_ids, *, tx):
        pairs = tx.with_table_scope(os_product_family_versions_table) \
            .select("SELECT id, logo_uri FROM $table "
                    "WHERE ? AND ? ",
                    SqlIn("status", _V_Status.PUBLIC),
                    SqlIn("id", version_ids))
        result = {}
        for p in pairs:
            result[p["id"]] = p["logo_uri"]
        return result

    @staticmethod
    @mkt_transaction()
    def rpc_set_logo_uri(version_id: ResourceIdType, uri: str, *, tx):
        version_update = {
            "logo_uri": uri,
        }
        tx.with_table_scope(os_product_family_versions_table) \
            .update_object("UPDATE $table $set WHERE id = ?",
                           version_update,
                           version_id)

    @staticmethod
    @mkt_transaction()
    def task_create(*, task_group_id, version_id, family_id, depends=None, tx) -> OsProductFamilyVersionOperation:
        if depends is None:
            depends = []
        return TaskUtils.create(
            "os_product_family_version_create",
            group_id=task_group_id,
            params=OsProductFamilyVersionCreateParams({"id": version_id}).to_primitive(),
            depends=depends,
            metadata=OsProductFamilyVersionMetadata({
                "os_product_family_id": family_id,
                "os_product_family_version_id": version_id,
            }).to_primitive(),
            tx=tx,
        )

    @staticmethod
    @mkt_transaction()
    def task_clone(*, task_group_id, version_id, image_id, family_slug, resource_spec, depends=None,
                   tx) -> OperationV1Beta1:
        if depends is None:
            depends = []
        return TaskUtils.create(
            "start_clone_image",
            params=StartCloneImageParams({
                "version_id": version_id,
                "source_image_id": image_id,
                "target_folder_id": os.getenv("MARKETPLACE_PENDING_IMG_FOLDER_ID"),
                "name": family_slug,
                "product_ids": [version_id],
                "resource_spec": resource_spec,
            }).to_primitive(),
            group_id=task_group_id,
            depends=depends,
            tx=tx,
        )

    @staticmethod
    @mkt_transaction()
    def task_finalize(*, task_group_id, version_id, family_id, target_status, depends=None,
                      tx) -> OsProductFamilyOperation:
        if depends is None:
            depends = []
        return TaskUtils.create(
            "finalize_clone_image",
            group_id=task_group_id,
            params=FinalizeImageParams({
                "version_id": version_id,
                "target_status": target_status,
            }).to_primitive(),
            metadata=OsProductFamilyVersionMetadata({
                "os_product_family_id": family_id,
                "os_product_family_version_id": version_id,
            }).to_primitive(),
            depends=depends,
            tx=tx,
        )

    @staticmethod
    @mkt_transaction()
    def task_bind_sku(*, task_group_id, version_id, depends=None, tx) -> OperationV1Beta1:
        if depends is None:
            depends = []
        return TaskUtils.create(
            "bind_skus_to_version",
            group_id=task_group_id,
            params=BindSkusToVersionParams({
                "version_id": version_id,
            }).to_primitive(),
            depends=depends,
            tx=tx,
        )

    @staticmethod
    @mkt_transaction()
    def task_publish(*, task_group_id, version_id, target_folder_id, depends=None, tx) -> OperationV1Beta1:
        if depends is None:
            depends = []
        return TaskUtils.create(
            "start_publish_image",
            group_id=task_group_id,
            params=StartPublishVersionParams({
                "version_id": version_id,
                "target_folder_id": target_folder_id,
            }).to_primitive(),
            depends=depends,
            tx=tx,
        )

    @staticmethod
    @mkt_transaction()
    def task_update_image_pool_size(*, task_group_id: str, pool_size: int,
                                    depends: list = None, tx):
        if depends is None:
            depends = []
        return lib.TaskUtils.create(
            "update_image_pool_size",
            group_id=task_group_id,
            tx=tx,
            params=UpdateImagePoolSizeParams({
                "pool_size": pool_size,
            }).to_primitive(),
            depends=depends,
        )

    @staticmethod
    @mkt_transaction()
    def task_finalize_image_pool_size(*, task_group_id: str,
                                      depends: list = None, tx):
        if depends is None:
            depends = []
        return lib.TaskUtils.create(
            "finalize_image_pool_size",
            group_id=task_group_id,
            tx=tx,
            params={},
            depends=depends,
        )

    @staticmethod
    @mkt_transaction()
    def task_delete_disk_pooling(*, task_group_id: str, image_id,
                                 depends: list = None, tx):
        if depends is None:
            depends = []
        return lib.TaskUtils.create(
            "delete_image_pool",
            group_id=task_group_id,
            tx=tx,
            params=DeleteImagePoolParams({
                "image_id": image_id,
            }).to_primitive(),
            depends=depends,
        )

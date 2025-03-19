import time
from typing import List
from typing import Optional
from typing import Tuple
from typing import Union  # noqa: F401

from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import categories_table
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.db.models import ordering_table
from cloud.marketplace.common.yc_marketplace_common.db.models import saas_products_table
from cloud.marketplace.common.yc_marketplace_common.lib import Avatar
from cloud.marketplace.common.yc_marketplace_common.lib import Eula
from cloud.marketplace.common.yc_marketplace_common.lib.i18n import I18n
from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.models.product_type import ProductType
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProduct as SaasProductScheme
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductList
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductMetadata
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductResponse
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductUpdateRequest
from cloud.marketplace.common.yc_marketplace_common.models.task import SaasProductCreateOrUpdateParams
from cloud.marketplace.common.yc_marketplace_common.utils.errors import SaasProductIdError
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_filter_with_category
from cloud.marketplace.common.yc_marketplace_common.utils.filter import parse_order_by
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common.utils.paging import page_query_args_with_complex_cursor
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction
from yc_common import config
from yc_common import logging
from yc_common.clients.kikimr import ColumnStrippingStrategy
from yc_common.clients.kikimr import TransactionMode
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.misc import drop_none
from yc_common.paging import page_handler
from yc_common.validation import ResourceIdType

log = logging.get_logger('yc')


def get_default_category() -> str:
    return config.get_value("marketplace.default_saas_product_category",
                            default="xxx00000000000000000")


def with_default(ids: List[str] = None) -> List[str]:
    default_category = get_default_category()
    if ids is None:
        ids = []
    if default_category not in ids:
        return [default_category] + ids
    return ids


class SaasProduct:
    @staticmethod
    @mkt_transaction()
    def get(product_id: ResourceIdType, *, tx) -> SaasProductScheme:
        product = tx.with_table_scope(saas_products_table).select_one(
            "SELECT " + SaasProductScheme.db_fields() + " " +
            "FROM $table "
            "WHERE id = ?", product_id,
            model=SaasProductScheme)
        return product

    @staticmethod
    @mkt_transaction()
    def rpc_get(product_id: ResourceIdType, *, tx) -> SaasProductScheme:
        product = SaasProduct.get(product_id, tx=tx)
        if product is None:
            raise SaasProductIdError()
        return product

    @staticmethod
    @mkt_transaction()
    def rpc_get_full(slug: Union[ResourceIdType, str], *, tx) -> SaasProductResponse:
        product_id = lib.ProductSlug.get_product(slug, tx=tx)
        if product_id is None:
            product_id = slug  # for backward compatibility
        product = marketplace_db().with_table_scope(
            products=saas_products_table,
            order=ordering_table,
            categories=categories_table
        ).select_one(
            "SELECT o.category_ids, p.* "
            "FROM $products_table as p "
            "LEFT JOIN (SELECT List(ord.category_id) as category_ids, ord.resource_id as product_id "
            "FROM $order_table AS ord "
            "JOIN $categories_table as c "
            "ON c.id = ord.category_id "
            "WHERE c.type == ? "
            "GROUP BY ord.resource_id) as o "
            "ON o.product_id = p.id "
            "WHERE id = ?", Category.Type.PUBLIC, product_id,
            model=SaasProductResponse,
            ignore_unknown=True,
            strip_table_from_columns=ColumnStrippingStrategy.STRIP_AND_MERGE)
        if product is None:
            raise SaasProductIdError()
        product.slugs = lib.ProductSlug.get_product_slugs(product_id)
        # clean up redundant ids
        return product

    @staticmethod
    @page_handler(items="products")
    @mkt_transaction(tx_mode=TransactionMode.ONLINE_READ_ONLY_CONSISTENT)
    def rpc_public_list(cursor: Optional[str],
                        limit: Optional[int] = 100,
                        *,
                        tx,
                        order_by: Optional[str] = None,
                        filter_query: Optional[str] = None) -> SaasProductList:
        start_method_time = time.monotonic()
        where_query, where_args = SaasProduct._prepare_list_sql_conditions(cursor, filter_query, limit, order_by)
        start_query_time = time.monotonic()
        iterator = tx.with_table_scope(
            product=saas_products_table,
            ordering=ordering_table,
            category=categories_table,
        ).select(
            """
            SELECT product.*, sub.category_ids, ordering.`order`
            FROM $product_table as product
            JOIN $ordering_table as ordering ON ordering.resource_id = product.id
            LEFT JOIN (
              SELECT List(sub_ordering.category_id) as category_ids, sub_ordering.resource_id as product_id
              FROM $ordering_table AS sub_ordering
              JOIN $category_table as category ON category.id = sub_ordering.category_id
              WHERE category.type == ?
              GROUP BY sub_ordering.resource_id
            ) as sub ON sub.product_id = product.id """ + where_query,
            Category.Type.PUBLIC,
            *where_args,
            model=SaasProductResponse,
            ignore_unknown=True,
            strip_table_from_columns=ColumnStrippingStrategy.STRIP_AND_MERGE,
        )
        log.debug("Query rpc_public_list_1 time: %s ms" % (time.monotonic() - start_query_time))
        product_list = SaasProduct._enrich_product_list(iterator, limit)
        log.debug("Method rpc_public_list time: %s ms" % (time.monotonic() - start_method_time))
        return product_list

    @staticmethod
    @page_handler(items="products")
    def rpc_list(cursor: Optional[str],
                 limit: Optional[int] = 100,
                 *,
                 billing_account_id: ResourceIdType,
                 order_by: Optional[str] = None,
                 filter_query: Optional[str] = None) -> SaasProductList:
        where_query, where_args = SaasProduct._prepare_list_sql_conditions(cursor, filter_query, limit, order_by,
                                                                           False)
        log.info("where::  {}, {}".format(where_query, where_args))
        iterator = marketplace_db().with_table_scope(products=saas_products_table,
                                                     order=ordering_table,
                                                     categories=categories_table).select(
            "SELECT  sub.category_ids, product.*, ordering.`order` "
            "FROM (SELECT * FROM $products_table WHERE billing_account_id = ?) as product "
            "JOIN $order_table as ordering ON ordering.resource_id = product.id "
            "LEFT JOIN ( "
            "    SELECT List(ord.category_id) as category_ids, ord.resource_id as product_id "
            "    FROM $order_table AS ord "
            "    JOIN $categories_table as c ON c.id = ord.category_id "
            "    WHERE c.type == ? "
            "    GROUP BY ord.resource_id "
            ") as sub "
            "ON sub.product_id = product.id " + where_query, billing_account_id, Category.Type.PUBLIC,
            *where_args, model=SaasProductResponse, ignore_unknown=True,
            strip_table_from_columns=ColumnStrippingStrategy.STRIP_AND_MERGE)
        product_list = SaasProduct._enrich_product_list(iterator, limit)
        return product_list

    @staticmethod
    @mkt_transaction()
    def get_batch(ids, *, tx) -> List[SaasProductScheme]:
        if not ids:
            return []

        return tx.with_table_scope(saas_products_table).select(
            "SELECT " + SaasProductScheme.db_fields() + " " +
            "FROM $table " +
            "WHERE ?", SqlIn('id', ids),
            model=SaasProductScheme)

    @staticmethod
    @mkt_transaction()
    def _enrich_product_list(iterator, limit, *, tx):
        start_method_time = time.monotonic()
        products = iterator
        p_ids = [p.id for p in products]
        slug_dict = lib.ProductSlug.get_products_slugs(p_ids, tx=tx)
        product_list = SaasProductList({
            "products": products,
        })
        if limit is not None and len(products) == limit:
            product_list.next_page_token = products[-1].id
        for p in product_list.products:
            p.slugs = slug_dict.get(p.id, [])
        log.debug("Method _enrich_product_list time: %s ms" % (time.monotonic() - start_method_time))
        return product_list

    @staticmethod
    def _prepare_list_sql_conditions(cursor: Optional[ResourceIdType],
                                     filter_query: Optional[str],
                                     limit: Optional[int],
                                     order_by: Optional[str],
                                     active=True) -> Tuple[str, list]:
        cursor_product = {}  # type: Union[dict, SaasProductResponse]
        filter_query_list, filter_args, category_id = parse_filter_with_category(filter_query, SaasProductScheme)
        mapping = {
            "order": "ordering.`order`",
            "id": "product.id",
            "createdAt": "product.created_at",
            "updatedAt": "product.updated_at",
            "name": "product.name",
            "description": "product.description",
            "shortDescription": "product.short_description",
            "status": "product.status",
            "billingAccountId": "product.billing_account_id",
        }
        order_by = parse_order_by(order_by, mapping, "order")
        if category_id is None:
            category_id = get_default_category()
        if cursor:
            product = marketplace_db().with_table_scope(products=saas_products_table,
                                                        order=ordering_table).select_one(
                "SELECT o.`order`, p.* "
                "FROM $products_table as p "
                "JOIN $order_table as o "
                "ON o.resource_id = p.id "
                "WHERE p.id = ? AND o.category_id = ? ", cursor, category_id,
                model=SaasProductResponse, ignore_unknown=True,
                strip_table_from_columns=ColumnStrippingStrategy.STRIP_AND_MERGE)
            if product is None:
                raise SaasProductIdError()
            cursor_product = product
        filter_query_list += ["ordering.category_id = ?"]
        filter_args += [category_id]
        if active:
            filter_query_list += ["product.status = ?"]
            filter_args += ["active"]
        if filter_query_list is not None:
            filter_query = " AND ".join(filter_query_list)
        where_query, where_args = page_query_args_with_complex_cursor(
            cursor,
            cursor_product,
            limit,
            mapping,
            id="id",
            filter_query=filter_query,
            filter_args=filter_args,
            order_by=order_by,
        )
        return where_query, where_args

    @staticmethod
    @mkt_transaction()
    def rpc_create(request: SaasProductCreateRequest, *, tx) -> SaasProductOperation:
        lib.Publisher.check_state(request.billing_account_id)
        meta = request.meta
        request.meta = {}
        product = SaasProductScheme.from_request(request)
        if request.slug is not None:
            lib.ProductSlug.add_slug(request.slug, product.id, product_type=ProductType.SAAS, tx=tx)
        for key in meta:
            product.meta[key] = I18n.set("saas_product.{}.meta.{}".format(product.id, key), meta[key])
        for field in {"name", "description", "short_description", "vendor"}:
            if hasattr(product, field):
                setattr(product, field, (getattr(product, field) or "").format(id=product.id))
        publish_logo = None
        publish_eula = None
        task_group_id = generate_id()
        if product.logo_id is not None:
            Avatar.capture_image(product.logo_id, product.id, tx=tx)
            publish_logo = Avatar.task_publish(
                product.logo_id,
                product.id,
                config.get_value("endpoints.s3.products_bucket"),
                group_id=task_group_id,
                tx=tx,
            )
        if product.eula_id is not None:
            Eula.link_agreement(product.eula_id, product.id, tx=tx)
            publish_eula = Eula.task_publish(
                product.eula_id,
                product.id,
                config.get_value("endpoints.s3.eulas_bucket"),
                group_id=task_group_id,
            )
        with marketplace_db().transaction() as tx:
            tx.with_table_scope(saas_products_table).insert_object("INSERT INTO $table", product)
            lib.Ordering.set_order(product.id, with_default(request.category_ids), tx=tx)
            lib.ProductToSkuBinding.update(product_id=product.id, sku_ids=request.sku_ids,
                                           publisher_id=product.billing_account_id)
        product_create_task_deps = []
        if publish_logo:
            product_create_task_deps += [publish_logo.id]
        if publish_eula:
            product_create_task_deps += [publish_eula.id]
        return SaasProduct.task_create(
            task_group_id=task_group_id,
            product_id=product.id,
            depends=product_create_task_deps)

    @staticmethod
    @mkt_transaction()
    def rpc_update(request: SaasProductUpdateRequest, *, tx) -> SaasProductOperation:
        product = SaasProduct.rpc_get_full(request.product_id)
        lib.Publisher.check_state(product.billing_account_id)

        product_update = drop_none({
            "labels": request.labels,
            "name": request.name,
            "description": request.description,
            "short_description": request.short_description,
            "logo_id": request.logo_id,
            "eula_id": request.eula_id,
            "vendor": request.vendor,
            "meta": request.meta,
            "sku_ids": request.sku_ids,
        })
        if request.slug is not None:
            lib.ProductSlug.add_slug(request.slug, product.id, product_type=ProductType.SAAS, tx=tx)
        publish_logo = None
        publish_eula = None
        task_group_id = generate_id()
        if product_update:
            for field in {"name", "description", "short_description", "vendor"}:
                if field in product_update:
                    product_update[field] = product_update[field].format(id=product.id)
            if "meta" in product_update:
                for key in product_update["meta"]:
                    product_update["meta"][key] = I18n.set("saas_product.{}.meta.{}".format(product.id, key),
                                                           product_update["meta"][key])
            if "logo_id" in product_update:
                if "logo_id" not in product or product["logo_id"] != product_update["logo_id"]:
                    if product_update["logo_id"] != "":
                        Avatar.capture_image(product_update["logo_id"], product.id, tx=tx)
                        # Product here has old id, so we need to pass logo id from update
                        publish_logo = Avatar.task_publish(
                            product_update["logo_id"],
                            product.id,
                            config.get_value("endpoints.s3.products_bucket"),
                            group_id=task_group_id,
                            rewrite=True,
                        )
                    else:
                        product_update["logo_uri"] = ""
            if "eula_id" in product_update:
                if "eula_id" not in product or product["eula_id"] != product_update["eula_id"]:
                    if product_update["eula_id"] != "":
                        Eula.link_agreement(product_update["eula_id"], product.id, tx=tx)
                        # Product here has old id, so we need to pass eula id from update
                        publish_eula = Eula.task_publish(
                            product_update["eula_id"],
                            product.id,
                            config.get_value("endpoints.s3.eulas_bucket"),
                            group_id=task_group_id,
                        )
                    else:
                        product_update["eula_uri"] = ""
            tx.with_table_scope(saas_products_table).update_object("UPDATE $table $set WHERE id = ?",
                                                                   product_update,
                                                                   request.product_id)
        if request.category_ids is not None:
            category_ids = with_default(request.category_ids)
            lib.Ordering.set_order(product.id, category_ids, tx=tx)

        lib.ProductToSkuBinding.update(product_id=product.id, sku_ids=request.sku_ids,
                                       publisher_id=product.billing_account_id)

        product_update_task_deps = []
        if publish_logo:
            product_update_task_deps += [publish_logo.id]
        if publish_eula:
            product_update_task_deps += [publish_eula.id]
        product_update_task = SaasProduct.task_update(
            task_group_id=task_group_id,
            product_id=product.id,
            depends=product_update_task_deps)
        return product_update_task

    @staticmethod
    @mkt_transaction()
    def rpc_set_logo_uri(product_id: ResourceIdType, uri: str, *, tx):
        product_update = {
            "logo_uri": uri,
        }
        tx.with_table_scope(saas_products_table) \
            .update_object("UPDATE $table $set WHERE id = ?",
                           product_update,
                           product_id)

    @staticmethod
    @mkt_transaction()
    def rpc_publish(product_id: ResourceIdType, *, tx):
        product_update = {
            "status": SaasProductScheme.Status.ACTIVE,
        }
        tx.with_table_scope(saas_products_table) \
            .update_object("UPDATE $table $set WHERE id = ?",
                           product_update,
                           product_id)

    @staticmethod
    @mkt_transaction()
    def task_create(*, task_group_id, product_id, depends, tx):
        return lib.TaskUtils.create(
            operation_type="saas_product_create",
            group_id=task_group_id,
            params=SaasProductCreateOrUpdateParams({"id": product_id}).to_primitive(),
            depends=depends,
            metadata=SaasProductMetadata({"saas_product_id": product_id}).to_primitive(),
            tx=tx,
        )

    @staticmethod
    @mkt_transaction()
    def task_update(*, task_group_id, product_id, depends, tx):
        return lib.TaskUtils.create(
            operation_type="saas_product_update",
            group_id=task_group_id,
            params=SaasProductCreateOrUpdateParams({"id": product_id}).to_primitive(),
            depends=depends,
            metadata=SaasProductMetadata({"saas_product_id": product_id}).to_primitive(),
            tx=tx,
        )

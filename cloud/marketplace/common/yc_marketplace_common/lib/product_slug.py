from yc_common.clients.kikimr.sql import SqlIn

from cloud.marketplace.common.yc_marketplace_common.db.models import product_slugs_table
from cloud.marketplace.common.yc_marketplace_common.models.product_slug import ProductSlugAddRequest
from cloud.marketplace.common.yc_marketplace_common.models.product_slug import ProductSlugRemoveRequest
from cloud.marketplace.common.yc_marketplace_common.models.product_slug import ProductSlugResponse
from cloud.marketplace.common.yc_marketplace_common.models.product_slug import ProductSlugScheme
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction


class ProductSlug:
    @classmethod
    @mkt_transaction()
    def get_product(cls, slug, *, tx):
        product_slug = tx.with_table_scope(product_slugs_table).select_one(
            "SELECT {} FROM $table WHERE slug = ?".format(ProductSlugScheme.db_fields()), slug,
            model=ProductSlugScheme)
        return product_slug.product_id if product_slug is not None else None

    @classmethod
    @mkt_transaction()
    def get_product_slugs(cls, product_id, *, tx):
        slugs = tx.with_table_scope(product_slugs_table).select(
            "SELECT {} FROM $table WHERE product_id = ?".format(ProductSlugScheme.db_fields()), product_id,
            model=ProductSlugScheme)
        return [ProductSlugResponse({"slug": slug.slug}) for slug in slugs]

    @classmethod
    @mkt_transaction()
    def get_products_slugs(cls, product_ids, *, tx):
        slugs = tx.with_table_scope(product_slugs_table).select(
            "SELECT {} FROM $table WHERE ?".format(ProductSlugScheme.db_fields()), SqlIn("product_id", product_ids),
            model=ProductSlugScheme)
        resp = {}
        for slug in slugs:
            if slug.product_id not in resp:
                resp[slug.product_id] = [ProductSlugResponse({"slug": slug.slug})]
            else:
                resp[slug.product_id] += [ProductSlugResponse({"slug": slug.slug})]
        return resp

    @classmethod
    @mkt_transaction()
    def add_slug(cls, slug, product_id, product_type, *, tx):
        tx_scoped = tx.with_table_scope(product_slugs_table)
        if tx_scoped.select_one("SELECT 1 FROM $table WHERE slug = ? AND product_id = ?", slug, product_id) is None:
            tx_scoped.insert_object("INSERT INTO $table ",
                                    ProductSlugScheme.from_create_request(slug, product_id, product_type))

    @classmethod
    def rpc_add_slug(cls, request: ProductSlugAddRequest):
        cls.add_slug(request.slug, request.product_id, request.product_type)
        return ProductSlugResponse({"slug": request.slug})

    @classmethod
    @mkt_transaction()
    def rpc_del_slug(cls, request: ProductSlugRemoveRequest, *, tx):
        tx.with_table_scope(product_slugs_table).query("DELETE FROM $table WHERE slug = ?", request.slug)
        return ProductSlugResponse({"slug": request.slug})

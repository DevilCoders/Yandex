from cloud.marketplace.common.yc_marketplace_common.models.avatar import AvatarResponse
from cloud.marketplace.common.yc_marketplace_common.models.billing.publisher_account import VAT
from cloud.marketplace.common.yc_marketplace_common.models.billing.revenue_report import RevenueMetaResponse
from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryList
from cloud.marketplace.common.yc_marketplace_common.models.deprecation import Deprecation
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvList
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvOperation
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvResponse
from cloud.marketplace.common.yc_marketplace_common.models.metrics import ServiceMetricsResponse
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductList
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductResponse
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyList
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyOperation
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyResponse
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersionList
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionResponse
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherList
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherOperation
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherResponse
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductList
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductResponse
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductList
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductResponse
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import CreateSkuDraftRequest
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftList
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftOperation
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftResponse
from cloud.marketplace.common.yc_marketplace_common.models.var import VarList
from cloud.marketplace.common.yc_marketplace_common.models.var import VarOperation
from cloud.marketplace.common.yc_marketplace_common.models.var import VarResponse
from yc_common.clients.api import ApiClient
from yc_common.misc import drop_none
from yc_common.misc import timestamp


class MarketplaceClient(object):
    def __init__(self, api_url, iam_token=None, timeout=10):
        self.__client = ApiClient(api_url,
                                  iam_token=iam_token,
                                  timeout=timeout)

    """Isv"""

    def get_isv(self, id) -> IsvResponse:
        url = "/isvs/{}".format(id)
        return self.__client.get(url, model=IsvResponse)

    def list_isvs(self, page_size=None, page_token=None, filter_query=None, order_by=None) -> IsvList:
        url = "/isvs"
        return self.__client.get(url, params=drop_none({
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
            "filter": filter_query,
            "orderBy": order_by,
        }), model=IsvList)

    def get_manage_isv(self, id) -> IsvResponse:
        url = "/manage/isvs/{}".format(id)
        return self.__client.get(url, model=IsvResponse)

    def list_manage_isvs(self, billing_account_id=None, page_size=None, page_token=None, filter_query=None,
                         order_by=None) -> IsvList:
        url = "/manage/isvs"
        return self.__client.get(url, params=drop_none({
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
            "filter": filter_query,
            "orderBy": order_by,
            "billingAccountId": billing_account_id,
        }), model=IsvList)

    def create_isv(self,
                   name,
                   logo_id=None,
                   description="",
                   contact_info=None,
                   billing_account_id=None,
                   meta=None) -> IsvOperation:
        contact_info = contact_info or {}
        url = "/manage/isvs"
        return self.__client.post(url, drop_none({
            "name": name,
            "description": description,
            "contactInfo": contact_info,
            "logoId": logo_id,
            "billing_account_id": billing_account_id,
            "meta": meta,
        }), model=IsvOperation)

    def update_isv(self,
                   id,
                   name=None,
                   logo_id=None,
                   description=None,
                   contact_info=None,
                   billing_account_id=None,
                   meta=None) -> IsvOperation:
        url = "/manage/isvs/{}".format(id)
        return self.__client.patch(url, drop_none({
            "name": name,
            "description": description,
            "contactInfo": contact_info,
            "logoId": logo_id,
            "billing_account_id": billing_account_id,
            "meta": meta,
        }), model=IsvOperation)

    """Var"""

    def get_var(self, id) -> VarResponse:
        url = "/vars/{}".format(id)
        return self.__client.get(url, model=VarResponse)

    def list_vars(self, page_size=None, page_token=None, filter_query=None, order_by=None) -> VarList:
        url = "/vars"
        return self.__client.get(url, params=drop_none({
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
            "filter": filter_query,
            "orderBy": order_by,
        }), model=VarList)

    def get_manage_var(self, id) -> VarResponse:
        url = "/manage/vars/{}".format(id)
        return self.__client.get(url, model=VarResponse)

    def list_manage_vars(self, billing_account_id=None, page_size=None, page_token=None, filter_query=None,
                         order_by=None) -> VarList:
        url = "/manage/vars"
        return self.__client.get(url, params=drop_none({
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
            "filter": filter_query,
            "orderBy": order_by,
            "billingAccountId": billing_account_id,
        }), model=VarList)

    def create_var(self,
                   name,
                   logo_id=None,
                   description="",
                   contact_info=None,
                   billing_account_id=None,
                   meta=None) -> VarOperation:
        contact_info = contact_info or {}
        url = "/manage/vars"
        return self.__client.post(url, drop_none({
            "name": name,
            "description": description,
            "contactInfo": contact_info,
            "logoId": logo_id,
            "billing_account_id": billing_account_id,
            "meta": meta,
        }), model=VarOperation)

    def update_var(self,
                   id,
                   name=None,
                   logo_id=None,
                   description=None,
                   contact_info=None,
                   billing_account_id=None,
                   meta=None) -> VarOperation:
        url = "/manage/vars/{}".format(id)
        return self.__client.patch(url, drop_none({
            "name": name,
            "description": description,
            "contactInfo": contact_info,
            "logoId": logo_id,
            "billing_account_id": billing_account_id,
            "meta": meta,
        }), model=VarOperation)

    """ Publisher """

    def get_publisher(self, id) -> PublisherResponse:
        url = "/publishers/{}".format(id)
        return self.__client.get(url, model=PublisherResponse)

    def list_publishers(self, page_size=None, page_token=None, filter_query=None, order_by=None) -> PublisherList:
        url = "/publishers"
        return self.__client.get(url, params=drop_none({
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
            "filter": filter_query,
            "orderBy": order_by,
        }), model=PublisherList)

    def get_manage_publisher(self, id) -> PublisherResponse:
        url = "/manage/publishers/{}".format(id)
        return self.__client.get(url, model=PublisherResponse)

    def list_manage_publishers(self, billing_account_id=None, page_size=None, page_token=None, filter_query=None,
                               order_by=None) -> PublisherList:
        url = "/manage/publishers"
        return self.__client.get(url, params=drop_none({
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
            "filter": filter_query,
            "orderBy": order_by,
            "billingAccountId": billing_account_id,
        }), model=PublisherList)

    def create_publisher(self,
                         billing_account_id,
                         name,
                         logo_id=None,
                         description="",
                         contact_info=None,
                         billing=None,
                         meta=None,
                         vat=VAT.RU_DEFAULT) -> PublisherOperation:
        contact_info = contact_info or {}
        url = "/manage/publishers"
        return self.__client.post(url, drop_none({
            "billing_account_id": billing_account_id,
            "name": name,
            "description": description,
            "contactInfo": contact_info,
            "logoId": logo_id,
            "billing": billing,
            "meta": meta,
            "vat": vat
        }), model=PublisherOperation)

    def update_publisher(self,
                         id,
                         name=None,
                         logo_id=None,
                         description=None,
                         contact_info=None,
                         billing=None,
                         meta=None) -> PublisherOperation:
        url = "/manage/publishers/{}".format(id)
        return self.__client.patch(url, drop_none({
            "name": name,
            "description": description,
            "contactInfo": contact_info,
            "logoId": logo_id,
            "billing": billing,
            "meta": meta,
        }), model=PublisherOperation)

    """saas product"""

    def get_public_saas_product(self, product_id) -> SaasProductResponse:
        url = "/saasProducts/{}".format(product_id)
        return self.__client.get(url, model=SaasProductResponse)

    def list_public_saas_products(self, page_size=None, page_token=None, filter_query=None,
                                  order_by=None) -> SaasProductList:
        url = "/saasProducts"
        return self.__client.get(url, params=drop_none({
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
            "filter": filter_query,
            "orderBy": order_by,
        }), model=SaasProductList)

    def get_saas_product(self, product_id) -> SaasProductResponse:
        url = "/manage/saasProducts/{}".format(product_id)
        return self.__client.get(url, model=SaasProductResponse)

    def create_saas_product(self, name,
                            short_description="",
                            description="",
                            logo_id=None,
                            eula_id=None,
                            labels=None,
                            category_ids=None,
                            billing_account_id=None,
                            vendor=None,
                            meta=None,
                            slug=None,
                            sku_ids=None) -> SaasProductOperation:
        url = "/manage/saasProducts"
        return self.__client.post(url, drop_none({
            "name": name,
            "labels": labels,
            "description": description,
            "shortDescription": short_description,
            "logoId": logo_id,
            "eulaId": eula_id,
            "categoryIds": category_ids,
            "billingAccountId": billing_account_id,
            "vendor": vendor,
            "meta": meta,
            "slug": slug,
            "sku_ids": sku_ids,
        }), model=SaasProductOperation)

    def list_saas_products(self, billing_account_id, page_size=None, page_token=None, filter_query=None,
                           order_by=None) -> SaasProductList:
        url = "/manage/saasProducts"
        return self.__client.get(url, params=drop_none({
            "billingAccountId": billing_account_id,
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
            "filter": filter_query,
            "orderBy": order_by,
        }), model=SaasProductList)

    def update_saas_product(self, product_id,
                            name=None,
                            labels=None,
                            description=None,
                            short_description=None,
                            logo_id=None,
                            eula_id=None,
                            primary_family_id=None,
                            category_ids=None,
                            vendor=None,
                            meta=None,
                            slug=None,
                            sku_ids=None) -> SaasProductOperation:
        url = "/manage/saasProducts/{}".format(product_id)
        return self.__client.patch(url, drop_none({
            "labels": labels,
            "name": name,
            "description": description,
            "shortDescription": short_description,
            "logoId": logo_id,
            "eulaId": eula_id,
            "primaryFamilyId": primary_family_id,
            "categoryIds": category_ids,
            "vendor": vendor,
            "meta": meta,
            "slug": slug,
            "sku_ids": sku_ids,
        }), model=SaasProductOperation)

    """simple product"""

    def get_public_simple_product(self, product_id) -> SimpleProductResponse:
        url = "/simpleProducts/{}".format(product_id)
        return self.__client.get(url, model=SimpleProductResponse)

    def list_public_simple_products(self, page_size=None, page_token=None, filter_query=None,
                                    order_by=None) -> SimpleProductList:
        url = "/simpleProducts"
        return self.__client.get(url, params=drop_none({
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
            "filter": filter_query,
            "orderBy": order_by,
        }), model=SimpleProductList)

    def get_simple_product(self, product_id) -> SimpleProductResponse:
        url = "/manage/simpleProducts/{}".format(product_id)
        return self.__client.get(url, model=SimpleProductResponse)

    def create_simple_product(self, name,
                              short_description="",
                              description="",
                              logo_id=None,
                              eula_id=None,
                              labels=None,
                              category_ids=None,
                              billing_account_id=None,
                              vendor=None,
                              meta=None,
                              slug=None) -> SimpleProductOperation:
        url = "/manage/simpleProducts"
        return self.__client.post(url, drop_none({
            "name": name,
            "labels": labels,
            "description": description,
            "shortDescription": short_description,
            "logoId": logo_id,
            "eulaId": eula_id,
            "categoryIds": category_ids,
            "billingAccountId": billing_account_id,
            "vendor": vendor,
            "meta": meta,
            "slug": slug,
        }), model=SimpleProductOperation)

    def list_simple_products(self, billing_account_id, page_size=None, page_token=None, filter_query=None,
                             order_by=None) -> SimpleProductList:
        url = "/manage/simpleProducts"
        return self.__client.get(url, params=drop_none({
            "billingAccountId": billing_account_id,
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
            "filter": filter_query,
            "orderBy": order_by,
        }), model=SimpleProductList)

    def update_simple_product(self, product_id,
                              name=None,
                              labels=None,
                              description=None,
                              short_description=None,
                              logo_id=None,
                              eula_id=None,
                              primary_family_id=None,
                              category_ids=None,
                              vendor=None,
                              meta=None,
                              slug=None) -> SimpleProductOperation:
        url = "/manage/simpleProducts/{}".format(product_id)
        return self.__client.patch(url, drop_none({
            "labels": labels,
            "name": name,
            "description": description,
            "shortDescription": short_description,
            "logoId": logo_id,
            "eulaId": eula_id,
            "primaryFamilyId": primary_family_id,
            "categoryIds": category_ids,
            "vendor": vendor,
            "meta": meta,
            "slug": slug,
        }), model=SimpleProductOperation)

    """product"""

    def get_public_os_product(self, product_id) -> OsProductResponse:
        url = "/osProducts/{}".format(product_id)
        return self.__client.get(url, model=OsProductResponse)

    def list_public_os_products(self, page_size=None, page_token=None, filter_query=None,
                                order_by=None, lang="en") -> OsProductList:
        url = "/osProducts"
        return self.__client.get(url, params=drop_none({
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
            "filter": filter_query,
            "orderBy": order_by,
        }), extra_headers={"Accept-Language": lang}, model=OsProductList)

    def get_batch_public_os_products(self, ids=None):
        url = "/osProducts/batch"
        return self.__client.get(url, params=drop_none({
            "ids": ",".join(ids),
        }))

    def get_os_product(self, product_id) -> OsProductResponse:
        url = "/manage/osProducts/{}".format(product_id)
        return self.__client.get(url, model=OsProductResponse)

    def create_os_product(self, name,
                          short_description="",
                          description="",
                          logo_id=None,
                          labels=None,
                          category_ids=None,
                          billing_account_id=None,
                          vendor=None,
                          meta=None,
                          slug=None) -> OsProductOperation:
        url = "/manage/osProducts"
        return self.__client.post(url, drop_none({
            "name": name,
            "labels": labels,
            "description": description,
            "shortDescription": short_description,
            "logoId": logo_id,
            "categoryIds": category_ids,
            "billingAccountId": billing_account_id,
            "vendor": vendor,
            "meta": meta,
            "slug": slug,
        }), model=OsProductOperation)

    def list_os_products(self, billing_account_id, page_size=None, page_token=None, filter_query=None,
                         order_by=None) -> OsProductList:
        url = "/manage/osProducts"
        return self.__client.get(url, params=drop_none({
            "billingAccountId": billing_account_id,
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
            "filter": filter_query,
            "orderBy": order_by,
        }), model=OsProductList)

    def update_os_product(self, product_id,
                          name=None,
                          labels=None,
                          description=None,
                          short_description=None,
                          logo_id=None,
                          primary_family_id=None,
                          category_ids=None,
                          vendor=None,
                          meta=None,
                          slug=None) -> OsProductOperation:
        url = "/manage/osProducts/{}".format(product_id)
        return self.__client.patch(url, drop_none({
            "labels": labels,
            "name": name,
            "description": description,
            "shortDescription": short_description,
            "logoId": logo_id,
            "primaryFamilyId": primary_family_id,
            "categoryIds": category_ids,
            "vendor": vendor,
            "meta": meta,
            "slug": slug,
        }), model=OsProductOperation)

    """product family"""

    def get_public_os_product_family(self, product_id) -> OsProductFamilyResponse:
        url = "/osProductFamilies/{}".format(product_id)
        return self.__client.get(url, model=OsProductFamilyResponse)

    def list_public_os_product_families(self, filter_query=None, page_size=None,
                                        page_token=None) -> OsProductFamilyList:
        url = "/osProductFamilies"
        return self.__client.get(url, params=drop_none({
            "filter": filter_query,
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
        }), model=OsProductFamilyList)

    def get_os_product_family(self, product_id) -> OsProductFamilyResponse:
        url = "/manage/osProductFamilies/{}".format(product_id)
        return self.__client.get(url, model=OsProductFamilyResponse)

    def create_os_product_family(self, name, image_id, pricing_options,
                                 resource_spec, os_product_id, logo_id=None, eula_id=None, billing_account_id=None,
                                 skus: list = None, slug=None, meta=None, form_id=None, related_products=None,
                                 license_rules=None) -> OsProductFamilyOperation:
        url = "/manage/osProductFamilies"
        return self.__client.post(url, drop_none({
            "billingAccountId": billing_account_id,
            "name": name,
            "imageId": image_id,
            "pricingOptions": pricing_options,
            "resourceSpec": resource_spec,
            "osProductId": os_product_id,
            "logoId": logo_id,
            "eulaId": eula_id,
            "skus": skus,
            "slug": slug,
            "meta": meta,
            "formId": form_id,
            "related_products": related_products,
            "license_rules": license_rules,
        }), model=OsProductFamilyOperation)

    def deprecate_os_product_family(self, os_product_family_id, description="", status=Deprecation.Status.DEPRECATED,
                                    deprecated_at=timestamp(), replacement_uri=""):
        url = "/manage/osProductFamilies/{}:deprecate".format(os_product_family_id)
        return self.__client.post(url, {
            "deprecation": {
                "status": status,
                "description": description,
                "deprecatedAt": deprecated_at,
                "replacementUri": replacement_uri,
            },
        }, model=OsProductFamilyOperation)

    def list_os_product_families(self,
                                 billing_account_id,
                                 filter_query=None,
                                 page_size=None,
                                 page_token=None) -> OsProductFamilyList:
        url = "/manage/osProductFamilies"
        return self.__client.get(url, params=drop_none({
            "billingAccountId": billing_account_id,
            "filter": filter_query,
            "pageSize": str(page_size) if page_size else None,
            "pageToken": page_token,
        }), model=OsProductFamilyList)

    def update_os_product_family(self, family_id, name=None, description=None, image_id=None, pricing_options=None,
                                 resource_spec=None, logo_id=None, eula_id=None,
                                 skus: list = None, slug=None, form_id=None, related_products=None,
                                 license_rules=None) -> OsProductFamilyOperation:
        url = "/manage/osProductFamilies/{}".format(family_id)
        return self.__client.patch(url, drop_none({
            "name": name,
            "description": description,
            "imageId": image_id,
            "pricingOptions": pricing_options,
            "resource_spec": resource_spec,
            "logoId": logo_id,
            "eulaId": eula_id,
            "skus": skus,
            "slug": slug,
            "formId": form_id,
            "related_products": related_products,
            "license_rules": license_rules,
        }), model=OsProductFamilyOperation)

    """product family version"""

    def list_os_product_family_version(self, billing_account_id, filter_query=None) -> OsProductFamilyVersionList:
        url = "/manage/osProductFamilyVersions"
        params = {
            "billingAccountId": billing_account_id,
            "filter": filter_query,
        }
        return self.__client.get(url, params=params, model=OsProductFamilyVersionList)

    def get_os_product_family_version(self, version_id) -> OsProductFamilyVersionResponse:
        url = "/osProductFamilyVersions/{}".format(version_id)
        return self.__client.get(url, model=OsProductFamilyVersionResponse)

    def batch_os_product_family_version_logo_uri_resolve(self, ids: list) -> dict:
        url = "/osProductFamilyVersions/batchLogoUriResolve"
        params = {
            "ids": ",".join(ids),
        }
        return self.__client.get(url, params=params)

    """ categories """

    def get_category(self, category_id) -> Category:
        url = "/categories/{}".format(category_id)
        return self.__client.get(url, model=Category)

    def list_categories(self, filter_query=None) -> CategoryList:
        url = "/categories"
        params = drop_none({
            "filter": filter_query,
        })
        return self.__client.get(url, params=params, model=CategoryList)

    def list_os_products_categories(self) -> CategoryList:
        url = "/categories/osProducts"
        return self.__client.get(url, params={}, model=CategoryList)

    def list_isv_categories(self) -> CategoryList:
        url = "/categories/isvs"
        return self.__client.get(url, params={}, model=CategoryList)

    def list_var_categories(self) -> CategoryList:
        url = "/categories/vars"
        return self.__client.get(url, params={}, model=CategoryList)

    def list_publisher_categories(self) -> CategoryList:
        url = "/categories/publishers"
        return self.__client.get(url, params={}, model=CategoryList)

    def upload_avatar(self, filename, datastream) -> AvatarResponse:
        url = "/manage/avatars"
        self.__client._set_json_requests(False)

        try:
            res = self.__client.post(url, {}, files={"avatar": (filename, datastream)},
                                     model=AvatarResponse)
        except Exception as error:
            self.__client._set_json_requests(True)
            raise error
        finally:
            self.__client._set_json_requests(True)
        return res

    """SKU Draft"""

    def create_sku_draft(self, req: CreateSkuDraftRequest) -> SkuDraftOperation:
        print(req.to_api(False))
        return self.__client.post("/manage/skuDrafts", req.to_api(False), model=SkuDraftOperation)

    def get_sku_draft(self, sku_draft_id) -> SkuDraftResponse:
        url = "/manage/skuDrafts/{}".format(sku_draft_id)
        return self.__client.get(url, model=SkuDraftResponse)

    def list_sku_drafts(self, billing_account_id, filter_query=None) -> SkuDraftList:
        params = drop_none({
            "billingAccountId": billing_account_id,
            "filter": filter_query,
        })
        return self.__client.get("/manage/skuDrafts", params=params, model=SkuDraftList)

    """Revenue reports"""

    def get_revenue_reports_meta(self, publisher_id) -> RevenueMetaResponse:
        url = "/manage/publishers/{}/revenueReportsMeta".format(publisher_id)
        return self.__client.get(url, model=RevenueMetaResponse)

    """Metrics"""

    def simulate_billing_metrics(self, request) -> ServiceMetricsResponse:
        url = "/simulateBillingMetrics"
        return self.__client.post(url, request, model=ServiceMetricsResponse)

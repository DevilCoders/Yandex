from yc_common.clients.api import ApiClient
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.misc import drop_none

from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintList
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintOperation
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintResponse
from cloud.marketplace.common.yc_marketplace_common.models.build import BuildList
from cloud.marketplace.common.yc_marketplace_common.models.build import BuildOperation
from cloud.marketplace.common.yc_marketplace_common.models.build import BuildResponse
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryOperation
from cloud.marketplace.common.yc_marketplace_common.models.form import FormCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.form import FormList
from cloud.marketplace.common.yc_marketplace_common.models.form import FormResponse
from cloud.marketplace.common.yc_marketplace_common.models.form import FormUpdateRequest
from cloud.marketplace.common.yc_marketplace_common.models.health import HealthCheck
from cloud.marketplace.common.yc_marketplace_common.models.i18n import BulkI18nCreatePublic
from cloud.marketplace.common.yc_marketplace_common.models.operation import OperationList
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionOperation
from cloud.marketplace.common.yc_marketplace_common.models.partner_requests import PartnerRequestPublicList
from cloud.marketplace.common.yc_marketplace_common.models.product_slug import ProductSlugResponse
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherOperation
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftList
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftOperation
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftResponse


class MarketplacePrivateClient(object):
    def __init__(self, api_url, iam_token, retry_temporary_errors=None, timeout=10):
        self.__client = ApiClient(api_url,
                                  iam_token=iam_token,
                                  timeout=timeout)
        self.__retry_temporary_errors = retry_temporary_errors

    """ Heath check """

    def health(self) -> HealthCheck:
        return self.__client.get("/health", model=HealthCheck)

    def ping(self):
        return self.__client.get("/ping")

    """ I18n """

    def bulk_set_i18n(self, lang_data):
        return self.__client.post("/i18n", lang_data, model=BulkI18nCreatePublic)

    def get_i18n(self, id, lang="ru"):
        return self.__client.get("/i18n", {"id": id}, extra_headers={"Accept-Language": lang})

    """ Form """

    def create_form(self, req: FormCreateRequest) -> OperationV1Beta1:
        return self.__client.post("/form", req.to_api(False), model=OperationV1Beta1)

    def update_form(self, req: FormUpdateRequest) -> OperationV1Beta1:
        data = req.to_api(False)
        id = data["id"]
        del data["id"]
        return self.__client.post("/form/{}".format(id), data, model=OperationV1Beta1)

    def get_form(self, id) -> FormResponse:
        return self.__client.get("/form/{}".format(id), model=FormResponse)

    def list_form(self):
        return self.__client.get("/form", model=FormList)

    """Publisher"""

    def update_publisher_status(self, id, status) -> PublisherOperation:
        url = "/publishers/{}:setStatus".format(id)
        return self.__client.post(url, {
            "status": status,
        }, model=PublisherOperation)

    """Slug"""

    def add_slug(self, slug, pid, product_type):
        url = "/productSlugs"
        return self.__client.post(url, {
            "slug": slug,
            "product_id": pid,
            "product_type": product_type,
        }, model=ProductSlugResponse)

    def del_slug(self, slug):
        url = "/productSlugs"
        return self.__client.delete(url, {
            "slug": slug,
        }, model=ProductSlugResponse)

    """Marketplace product version"""

    def approve_product_version(self, product_version_id) -> OperationV1Beta1:
        url = "/productVersions/{}/approve".format(product_version_id)
        return self.__client.post(url, {}, model=OperationV1Beta1)

    """Task"""

    def get_task(self, id):
        url = "/operations/{}".format(id)

        return self.__client.get(url, model=OperationV1Beta1)

    def list_tasks(self):
        url = "/operations"

        return self.__client.get(url, model=OperationList)

    def create_task(self, typ, params=None, group_id=None, is_infinite=False, depends=None):
        if depends is None:
            depends = []
        if params is None:
            params = {}
        return self.__client.post("/operations", {
            "type": typ,
            "groupId": group_id,
            "params": params,
            "isInfinite": is_infinite,
            "depends": depends,
        }, model=OperationV1Beta1)

    def cancel_task(self, id):
        return self.__client.post("/operations/{}:cancel".format(id), {}, model=OperationV1Beta1)

    """Saas Product"""

    def set_order_for_saas_product(self, product_id, order):
        url = "/saasProducts/{}:setOrder".format(product_id)
        return self.__client.post(url, drop_none({
            "order": order,
        }), model=SaasProductOperation)

    def put_saas_product_on_place(self, product_id, category_id, pos):
        url = "/saasProducts/{}:putOnPlace".format(product_id)
        return self.__client.post(url, {
            "categoryId": category_id,
            "position": pos,
        }, model=SaasProductOperation)

    def publish_saas_product(self, product_id):
        url = "/saasProducts/{}:publish".format(product_id)
        return self.__client.post(url, {}, model=SaasProductOperation)

    """Simple Product"""

    def set_order_for_simple_product(self, product_id, order):
        url = "/simpleProducts/{}:setOrder".format(product_id)
        return self.__client.post(url, drop_none({
            "order": order,
        }), model=SimpleProductOperation)

    def put_simple_product_on_place(self, product_id, category_id, pos):
        url = "/simpleProducts/{}:putOnPlace".format(product_id)
        return self.__client.post(url, {
            "categoryId": category_id,
            "position": pos,
        }, model=SimpleProductOperation)

    def publish_simple_product(self, product_id):
        url = "/simpleProducts/{}:publish".format(product_id)
        return self.__client.post(url, {}, model=SimpleProductOperation)

    """Product"""

    def set_order_for_os_product(self, product_id, order):
        url = "/osProducts/{}:setOrder".format(product_id)
        return self.__client.post(url, drop_none({
            "order": order,
        }), model=OsProductOperation)

    def put_os_product_on_place(self, product_id, category_id, pos):
        url = "/osProducts/{}:putOnPlace".format(product_id)
        return self.__client.post(url, {
            "categoryId": category_id,
            "position": pos,
        }, model=OsProductOperation)

    """product family version"""

    def set_status_product_family_version(self, version_id, status) -> OsProductFamilyVersionOperation:
        url = "/osProductFamilyVersions/{}:setStatus".format(version_id)

        return self.__client.post(url, {"status": status}, model=OsProductFamilyVersionOperation,
                                  retry_temporary_errors=self.__retry_temporary_errors)

    def set_image_id_product_family_version(self, version_id, image_id) -> OsProductFamilyVersionOperation:
        url = "/osProductFamilyVersions/{}:setImageId".format(version_id)

        return self.__client.post(url, {"imageId": image_id}, model=OsProductFamilyVersionOperation,
                                  retry_temporary_errors=self.__retry_temporary_errors)

    def publish_product_version(self, product_version_id, pool_size=1) -> OsProductFamilyVersionOperation:
        url = "/osProductFamilyVersions/{}:publish".format(product_version_id)
        return self.__client.post(url, {
            "poolSize": pool_size,
        }, model=OsProductFamilyVersionOperation)

    """Category"""

    def create_category(self, name, type, score, parent_id=None) -> CategoryOperation:
        url = "/categories"
        return self.__client.post(url, drop_none({
            "name": name,
            "type": type,
            "score": score,
            "parentId": parent_id,
        }), model=CategoryOperation)

    def update_category(self, category_id, name=None, type=None, score=None, parent_id=None) -> CategoryOperation:
        url = "/categories/{}".format(category_id)
        return self.__client.patch(url, drop_none({
            "name": name,
            "type": type,
            "score": score,
            "parentId": parent_id,
        }), model=CategoryOperation)

    def add_to_category(self, category_id, resource_ids) -> CategoryOperation:
        url = "/categories/{}:addResources".format(category_id)
        return self.__client.post(url, drop_none({
            "resourceIds": resource_ids,
        }), model=CategoryOperation)

    def remove_from_category(self, category_id, resource_ids) -> CategoryOperation:
        url = "/categories/{}:removeResources".format(category_id)
        return self.__client.post(url, drop_none({
            "resourceIds": resource_ids,
        }), model=CategoryOperation)

    def approve_partner_request(self, request_id):
        url = "/partner_requests/{}:approve".format(request_id)
        return self.__client.post(url, {})

    def decline_partner_request(self, request_id):
        url = "/partner_requests/{}:decline".format(request_id)
        return self.__client.post(url, {})

    def list_partner_request(self, request_id):
        url = "/partner_requests"
        return self.__client.get(url, model=PartnerRequestPublicList)

    """SKU Draft"""

    def accept_sku_draft(self, sku_draft_id) -> SkuDraftOperation:
        url = "/skuDrafts/{}:accept".format(sku_draft_id)
        return self.__client.post(url, {}, model=SkuDraftOperation)

    def reject_sku_draft(self, sku_draft_id) -> SkuDraftOperation:
        url = "/skuDrafts/{}:reject".format(sku_draft_id)
        return self.__client.post(url, {}, model=SkuDraftOperation)

    def list_sku_drafts(self, filter_query=None) -> SkuDraftList:
        params = drop_none({
            "filter": filter_query,
        })
        return self.__client.get("/skuDrafts", params=params, model=SkuDraftList)

    def get_sku_draft(self, sku_draft_id) -> SkuDraftResponse:
        url = "/skuDrafts/{}".format(sku_draft_id)
        return self.__client.get(url, model=SkuDraftResponse)

    """Blueprint"""

    def create_blueprint(self, req: BlueprintCreateRequest) -> BlueprintOperation:
        return self.__client.post("/blueprints", req.to_api(False), model=BlueprintOperation)

    def update_blueprint(self,
                         blueprint_id,
                         name=None,
                         commit_hash=None,
                         build_recipe_links=None,
                         test_suites_links=None,
                         test_instance_config=None) -> BlueprintOperation:
        url = "/blueprints/{}".format(blueprint_id)
        return self.__client.patch(url,
                                   drop_none({
                                       "name": name,
                                       "commit_hash": commit_hash,
                                       "build_recipe_links": build_recipe_links,
                                       "test_suites_links": test_suites_links,
                                       "test_instance_config": test_instance_config,
                                   }), model=BlueprintOperation)

    def get_blueprint(self, blueprint_id) -> BlueprintResponse:
        url = "/blueprints/{}".format(blueprint_id)
        return self.__client.get(url, model=BlueprintResponse)

    def list_blueprints(self, filter_query=None) -> BlueprintList:
        params = drop_none({
            "filter": filter_query,
        })
        return self.__client.get("/blueprints", params=params, model=BlueprintList)

    def accept_blueprint(self, blueprint_id) -> BlueprintOperation:
        url = "/blueprints/{}:accept".format(blueprint_id)
        return self.__client.post(url, {}, model=BlueprintOperation)

    def reject_blueprint(self, blueprint_id) -> BlueprintOperation:
        url = "/blueprints/{}:reject".format(blueprint_id)
        return self.__client.post(url, {}, model=BlueprintOperation)

    def build_blueprint(self, blueprint_id) -> BlueprintOperation:
        url = "/blueprints/{}:build".format(blueprint_id)
        return self.__client.post(url, {}, model=BlueprintOperation)

    """ Build """

    def get_build(self, build_id) -> BuildResponse:
        url = "/builds/{}".format(build_id)
        return self.__client.get(url, model=BuildResponse)

    def list_builds(self, filter_query=None) -> BuildList:
        params = drop_none({
            "filter": filter_query,
        })
        return self.__client.get("/builds", params=params, model=BuildList)

    def start_build(self, build_id) -> BuildOperation:
        url = "/builds/{}:start".format(build_id)
        return self.__client.post(url, {}, model=BuildOperation)

    def finish_build(self, build_id, compute_image_id, status) -> BuildOperation:
        url = "/builds/{}:finish".format(build_id)
        return self.__client.post(url,
                                  {
                                      "compute_image_id": compute_image_id,
                                      "status": status,
                                  }, model=BuildOperation)

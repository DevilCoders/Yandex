PY3TEST()

OWNER(g:cloud-marketplace)

USE_RECIPE(cloud/marketplace/misc/recipes/functest/functest)


TEST_SRCS(
    __init__.py
    conftest.py
    test_build.py
    test_blueprint.py
    test_category.py
    test_form.py
    test_health.py
    test_i18n.py
    test_isv.py
    test_product.py
    test_product_family.py
    test_product_family_version.py
    test_publisher.py
    test_saas_product.py
    test_simple_product.py
    test_sku_draft.py
    test_var.py
)

PEERDIR(
    cloud/marketplace/functests/yc_marketplace_functests
)

DEPENDS(
    cloud/marketplace/misc/recipes/functest
    cloud/marketplace/api/wsgi
    cloud/marketplace/cli/bin
    cloud/iam/accessservice/mock/python
)

SIZE(LARGE)
TAG(ya:fat ya:force_sandbox)
REQUIREMENTS(sb_vault:FUNC_TEST_OAUTH_TOKEN=file:YCMARKETPLACE:yndx-cloud-mkt-oauth)

END()

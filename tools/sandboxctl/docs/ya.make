MKDOCS(docs)

# FIXME, currently this variables is not used by DOCS macro
SET(DAAS_TEST_ID 1927)
SET(DAAS_PROD_ID 1201)
SET(DAAS_TEST_URL http://internal.daas-backend-int.locdoc-test.yandex.net/v1/projects/${DAAS_TEST_ID}/deploy)
SET(DAAS_PROD_URL http://internal.daas-backend-int.yandex.net/v1/projects/${DAAS_PROD_ID}/deply)

OWNER(dmtrmonakhov)
DOCS_CONFIG(config.yml)

END()

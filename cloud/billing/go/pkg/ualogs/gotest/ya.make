GO_TEST_FOR(cloud/billing/go/pkg/ualogs)

OWNER(g:cloud-billing)

ENV(UA_RECIPE_CONFIG_PATH=cloud/billing/go/pkg/ualogs/gotest/config.yml)

INCLUDE(${ARCADIA_ROOT}/logbroker/unified_agent/tools/ua_recipe/ua_recipe.inc)

TIMEOUT(590)

SIZE(MEDIUM)

END()

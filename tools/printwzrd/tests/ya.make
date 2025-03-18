OWNER(
    g:wizard
)

IF (AUTOCHECK)

RECURSE(
    test_antirobot
    test_default
    test_geo
    test_geosearch
)

ENDIF()

RECURSE_ROOT_RELATIVE(
    search/wizard/test_responses
)

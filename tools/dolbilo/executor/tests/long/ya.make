OWNER(
    g:base
    mvel
    darkk
)

IF (NOT AUTOCHECK) # these tests rely on execution time, using them at AUTOCHECK produces nothing but noise

    PY2TEST()

    TEST_SRCS(test_long.py)

    PEERDIR(tools/dolbilo/executor/tests/lib)

    SIZE(MEDIUM)

    TIMEOUT(360)

    DATA(
        arcadia/tools/dolbilo/executor/tests
        sbr://26292172 # https://sandbox.yandex-team.ru/resources?type=BASESEARCH_PLAN&attr_name=RusTier0_priemka&attr_value=1
    )

    DEPENDS(
        tools/dolbilo/planner
        tools/dolbilo/executor
        balancer/daemons/balancer
    )

    END()

ENDIF()

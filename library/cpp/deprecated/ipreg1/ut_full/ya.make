EXECTEST()

OWNER(
    g:geotargeting
)

DEPENDS(
    library/cpp/deprecated/ipreg1/ut_full/ipreg_checker
)

DATA(
    sbr://997552519 # IPREG_LAYOUT_STABLE; 'ipreg-layout.json'@STABLE; 2019-06-20 02:45:23.409675
)

RUN(
    STDOUT ${TEST_OUT_ROOT}/ipreg-checker.cout.txt
    STDERR ${TEST_OUT_ROOT}/ipreg-checker.cerr.txt

    ${ARCADIA_BUILD_ROOT}/library/cpp/deprecated/ipreg1/ut_full/ipreg_checker/ipreg_checker --ipreg-layout ${TEST_WORK_ROOT}/ipreg-layout.json --pairs 5 --crash-on-error
)

END()

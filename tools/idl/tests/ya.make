PY2TEST()

OWNER(
    g:geoapps_infra
)

SIZE(MEDIUM)

DEPENDS(
    tools/idl/bin
)

DATA(
    sbr://3373046272
)

TEST_SRCS(
    diff.py
)

END()

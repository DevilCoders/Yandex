OWNER(g:cloud-iam)

PY3_PROGRAM(planning_tool)

PEERDIR(
    contrib/python/colorama
    cloud/iam/planning_tool/library
)

PY_SRCS(
    MAIN planning_tool.py
)

END()

RECURSE(
    library
)

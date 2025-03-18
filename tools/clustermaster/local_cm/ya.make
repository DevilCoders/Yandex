UNION()

OWNER(
    g:clustermaster
    g:jupiter
)

BUNDLE(
    tools/clustermaster/local_cm/run_cm NAME local_cm
    tools/clustermaster/master
    tools/clustermaster/utils/cmcheck
    tools/clustermaster/worker
    tools/clustermaster/solver
)

END()

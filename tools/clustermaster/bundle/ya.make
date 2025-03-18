UNION()

OWNER(
    g:clustermaster
    g:jupiter
)

BUNDLE(
    tools/clustermaster/master
    tools/clustermaster/worker
    tools/clustermaster/solver
)

END()

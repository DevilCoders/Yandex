PY3_LIBRARY()

OWNER(g:mdb)

PEERDIR(
    contrib/python/django/django-3

    library/python/django
)

PY_SRCS(
    app.py
    urls.py
    filters.py
    models.py
    views/hosts.py
    views/clusters.py
    views/rollouts.py
    views/schedules.py
)

RESOURCE_FILES(
    cloud/mdb/backstage/apps/katan/templates/katan/clusters/clusters.html
    cloud/mdb/backstage/apps/katan/templates/katan/clusters/sections/common.html
    cloud/mdb/backstage/apps/katan/templates/katan/clusters/blocks/cluster_rollouts.html

    cloud/mdb/backstage/apps/katan/templates/katan/hosts/hosts.html
    cloud/mdb/backstage/apps/katan/templates/katan/hosts/sections/common.html
    cloud/mdb/backstage/apps/katan/templates/katan/hosts/blocks/shipments.html

    cloud/mdb/backstage/apps/katan/templates/katan/rollouts/rollouts.html
    cloud/mdb/backstage/apps/katan/templates/katan/rollouts/sections/common.html
    cloud/mdb/backstage/apps/katan/templates/katan/rollouts/blocks/cluster_rollouts.html
    cloud/mdb/backstage/apps/katan/templates/katan/rollouts/blocks/dependencies.html
    cloud/mdb/backstage/apps/katan/templates/katan/rollouts/blocks/shipments.html

    cloud/mdb/backstage/apps/katan/templates/katan/schedules/schedules.html
    cloud/mdb/backstage/apps/katan/templates/katan/schedules/sections/common.html
    cloud/mdb/backstage/apps/katan/templates/katan/schedules/blocks/rollouts.html
    cloud/mdb/backstage/apps/katan/templates/katan/schedules/blocks/convergence.html
    cloud/mdb/backstage/apps/katan/templates/katan/schedules/blocks/dependencies.html
)

END()

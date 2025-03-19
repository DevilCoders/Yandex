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
    views/clusters.py
    views/containers.py
    views/dom0_hosts.py
    views/projects.py
    views/reserved_resources.py
    views/transfers.py
)

RESOURCE_FILES(
    cloud/mdb/backstage/apps/dbm/templates/dbm/clusters/clusters.html
    cloud/mdb/backstage/apps/dbm/templates/dbm/clusters/sections/common.html

    cloud/mdb/backstage/apps/dbm/templates/dbm/containers/containers.html
    cloud/mdb/backstage/apps/dbm/templates/dbm/containers/sections/common.html

    cloud/mdb/backstage/apps/dbm/templates/dbm/dom0_hosts/dom0_hosts.html
    cloud/mdb/backstage/apps/dbm/templates/dbm/dom0_hosts/sections/common.html
    cloud/mdb/backstage/apps/dbm/templates/dbm/dom0_hosts/blocks/containers.html
    cloud/mdb/backstage/apps/dbm/templates/dbm/dom0_hosts/blocks/clusters.html
    cloud/mdb/backstage/apps/dbm/templates/dbm/dom0_hosts/blocks/resources.html

    cloud/mdb/backstage/apps/dbm/templates/dbm/projects/projects.html
    cloud/mdb/backstage/apps/dbm/templates/dbm/projects/sections/common.html
    cloud/mdb/backstage/apps/dbm/templates/dbm/projects/blocks/dom0_hosts.html

    cloud/mdb/backstage/apps/dbm/templates/dbm/reserved_resources/reserved_resources.html

    cloud/mdb/backstage/apps/dbm/templates/dbm/transfers/transfers.html
    cloud/mdb/backstage/apps/dbm/templates/dbm/transfers/sections/common.html
)

END()

PY3_LIBRARY()

OWNER(g:mdb)

PEERDIR(
    contrib/python/django/django-3
    contrib/python/jsonschema

    library/python/django
)

PY_SRCS(
    app.py
    urls.py
    models.py
    filters.py
    helpers.py
    views/commands.py
    views/groups.py
    views/minions.py
    views/shipment_commands.py
    views/masters.py
    views/shipments.py
    views/job_results.py
    templatetags/deploy/templatetags.py
)

RESOURCE_FILES(
    cloud/mdb/backstage/apps/deploy/templates/deploy/shipments/shipments.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/shipments/sections/common.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/shipments/blocks/progress.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/shipments/blocks/minions.html

    cloud/mdb/backstage/apps/deploy/templates/deploy/job_results/job_results.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/job_results/sections/common.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/job_results/blocks/job.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/job_results/includes/states_table.html

    cloud/mdb/backstage/apps/deploy/templates/deploy/commands/commands.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/commands/sections/common.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/commands/blocks/jobs.html

    cloud/mdb/backstage/apps/deploy/templates/deploy/groups/groups.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/groups/sections/common.html

    cloud/mdb/backstage/apps/deploy/templates/deploy/masters/masters.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/masters/sections/common.html

    cloud/mdb/backstage/apps/deploy/templates/deploy/minions/minions.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/minions/sections/common.html

    cloud/mdb/backstage/apps/deploy/templates/deploy/shipment_commands/shipment_commands.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/shipment_commands/sections/common.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/shipment_commands/blocks/job_results.html
    cloud/mdb/backstage/apps/deploy/templates/deploy/shipment_commands/blocks/commands.html
)

END()

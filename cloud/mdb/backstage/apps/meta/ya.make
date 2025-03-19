PY3_LIBRARY()

OWNER(g:mdb)

PEERDIR(
    contrib/python/django/django-3

    library/python/django
    cloud/mdb/backstage/contrib/dictdiffer
)

PY_SRCS(
    app.py
    urls.py
    models.py
    helpers.py
    filters.py
    views/folders.py
    views/backups.py
    views/versions.py
    views/default_versions.py
    views/shards.py
    views/subclusters.py
    views/valid_resources.py
    views/maintenance_tasks.py
    views/flavors.py
    views/hosts.py
    views/clouds.py
    views/clusters.py
    views/worker_tasks.py
    actions/worker_tasks.py
    templatetags/meta/templatetags.py
)

RESOURCE_FILES(
    cloud/mdb/backstage/apps/meta/templates/meta/clusters/clusters.html
    cloud/mdb/backstage/apps/meta/templates/meta/clusters/sections/common.html
    cloud/mdb/backstage/apps/meta/templates/meta/clusters/blocks/versions.html
    cloud/mdb/backstage/apps/meta/templates/meta/clusters/blocks/revs.html
    cloud/mdb/backstage/apps/meta/templates/meta/clusters/blocks/maintenance_tasks.html
    cloud/mdb/backstage/apps/meta/templates/meta/clusters/blocks/worker_tasks.html
    cloud/mdb/backstage/apps/meta/templates/meta/clusters/blocks/subclusters.html
    cloud/mdb/backstage/apps/meta/templates/meta/clusters/blocks/pillar.html
    cloud/mdb/backstage/apps/meta/templates/meta/clusters/blocks/pillar_revs.html
    cloud/mdb/backstage/apps/meta/templates/meta/clusters/blocks/hosts_health.html
    cloud/mdb/backstage/apps/meta/templates/meta/clusters/blocks/cluster_health.html

    cloud/mdb/backstage/apps/meta/templates/meta/worker_tasks/worker_tasks.html
    cloud/mdb/backstage/apps/meta/templates/meta/worker_tasks/sections/common.html
    cloud/mdb/backstage/apps/meta/templates/meta/worker_tasks/blocks/shipments.html
    cloud/mdb/backstage/apps/meta/templates/meta/worker_tasks/blocks/acquire_finish_changes.html
    cloud/mdb/backstage/apps/meta/templates/meta/worker_tasks/blocks/restarts.html
    cloud/mdb/backstage/apps/meta/templates/meta/worker_tasks/blocks/required_task.html
    cloud/mdb/backstage/apps/meta/templates/meta/worker_tasks/action_dialog.html


    cloud/mdb/backstage/apps/meta/templates/meta/clouds/clouds.html
    cloud/mdb/backstage/apps/meta/templates/meta/clouds/sections/common.html

    cloud/mdb/backstage/apps/meta/templates/meta/hosts/hosts.html
    cloud/mdb/backstage/apps/meta/templates/meta/hosts/sections/common.html
    cloud/mdb/backstage/apps/meta/templates/meta/hosts/blocks/mlock.html
    cloud/mdb/backstage/apps/meta/templates/meta/hosts/blocks/revs.html
    cloud/mdb/backstage/apps/meta/templates/meta/hosts/blocks/pillar.html
    cloud/mdb/backstage/apps/meta/templates/meta/hosts/blocks/pillar_revs.html
    cloud/mdb/backstage/apps/meta/templates/meta/hosts/blocks/cms_last_decision.html

    cloud/mdb/backstage/apps/meta/templates/meta/flavors/flavors.html
    cloud/mdb/backstage/apps/meta/templates/meta/flavors/sections/common.html

    cloud/mdb/backstage/apps/meta/templates/meta/folders/folders.html
    cloud/mdb/backstage/apps/meta/templates/meta/folders/sections/common.html

    cloud/mdb/backstage/apps/meta/templates/meta/backups/backups.html
    cloud/mdb/backstage/apps/meta/templates/meta/backups/sections/common.html

    cloud/mdb/backstage/apps/meta/templates/meta/versions/versions.html

    cloud/mdb/backstage/apps/meta/templates/meta/default_versions/default_versions.html

    cloud/mdb/backstage/apps/meta/templates/meta/maintenance_tasks/maintenance_tasks.html
    cloud/mdb/backstage/apps/meta/templates/meta/maintenance_tasks/sections/common.html

    cloud/mdb/backstage/apps/meta/templates/meta/shards/shards.html
    cloud/mdb/backstage/apps/meta/templates/meta/shards/sections/common.html
    cloud/mdb/backstage/apps/meta/templates/meta/shards/blocks/pillar.html
    cloud/mdb/backstage/apps/meta/templates/meta/shards/blocks/pillar_revs.html

    cloud/mdb/backstage/apps/meta/templates/meta/subclusters/subclusters.html
    cloud/mdb/backstage/apps/meta/templates/meta/subclusters/sections/common.html
    cloud/mdb/backstage/apps/meta/templates/meta/subclusters/blocks/pillar.html
    cloud/mdb/backstage/apps/meta/templates/meta/subclusters/blocks/pillar_revs.html

    cloud/mdb/backstage/apps/meta/templates/meta/valid_resources/valid_resources.html
    cloud/mdb/backstage/apps/meta/templates/meta/valid_resources/sections/common.html

    cloud/mdb/backstage/apps/meta/templates/meta/includes/hosts.html
    cloud/mdb/backstage/apps/meta/templates/meta/includes/cluster_with_type.html
    cloud/mdb/backstage/apps/meta/templates/meta/includes/pillar_revs_block.html
    cloud/mdb/backstage/apps/meta/templates/meta/includes/task_restart_changes.html
)

END()

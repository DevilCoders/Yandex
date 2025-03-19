import django.urls as du

import cloud.mdb.backstage.apps.meta.views.clusters as clusters
import cloud.mdb.backstage.apps.meta.views.worker_tasks as worker_tasks
import cloud.mdb.backstage.apps.meta.views.clouds as clouds
import cloud.mdb.backstage.apps.meta.views.hosts as hosts
import cloud.mdb.backstage.apps.meta.views.flavors as flavors
import cloud.mdb.backstage.apps.meta.views.folders as folders
import cloud.mdb.backstage.apps.meta.views.backups as backups
import cloud.mdb.backstage.apps.meta.views.versions as versions
import cloud.mdb.backstage.apps.meta.views.default_versions as default_versions
import cloud.mdb.backstage.apps.meta.views.maintenance_tasks as maintenance_tasks
import cloud.mdb.backstage.apps.meta.views.shards as shards
import cloud.mdb.backstage.apps.meta.views.subclusters as subclusters
import cloud.mdb.backstage.apps.meta.views.valid_resources as valid_resources


urlpatterns = [
    du.path('clouds', clouds.CloudListSkelView.as_view()),
    du.path('clouds/<str:cloud_id>', clouds.CloudSkelView.as_view()),
    du.path('clouds/<str:cloud_id>/sections/<str:section>', clouds.CloudSkelView.as_view()),

    du.path('hosts', hosts.HostListSkelView.as_view()),
    du.path('hosts/<str:fqdn>', hosts.HostSkelView.as_view()),
    du.path('hosts/<str:fqdn>/sections/<str:section>', hosts.HostSkelView.as_view()),

    du.path('clusters', clusters.ClusterListSkelView.as_view()),
    du.path('clusters/<str:cluster_id>', clusters.ClusterSkelView.as_view()),
    du.path('clusters/<str:cluster_id>/sections/<str:section>', clusters.ClusterSkelView.as_view()),

    du.path('worker_tasks', worker_tasks.TaskListSkelView.as_view()),
    du.path('worker_tasks/<str:task_id>', worker_tasks.TaskSkelView.as_view()),
    du.path('worker_tasks/<str:task_id>/sections/<str:section>', worker_tasks.TaskSkelView.as_view()),

    du.path('flavors', flavors.FlavorListSkelView.as_view()),
    du.path('flavors/<str:id>', flavors.FlavorSkelView.as_view()),
    du.path('flavors/<str:id>/sections/<str:section>', flavors.FlavorSkelView.as_view()),

    du.path('folders', folders.FolderListSkelView.as_view()),
    du.path('folders/<str:folder_id>', folders.FolderSkelView.as_view()),
    du.path('folders/<str:folder_id>/sections/<str:section>', folders.FolderSkelView.as_view()),

    du.path('maintenance_tasks', maintenance_tasks.MaintenanceTaskListSkelView.as_view()),
    du.path('maintenance_tasks/<str:task_id>', maintenance_tasks.MaintenanceTaskSkelView.as_view()),
    du.path('maintenance_tasks/<str:task_id>/sections/<str:section>', maintenance_tasks.MaintenanceTaskSkelView.as_view()),

    du.path('backups', backups.BackupListSkelView.as_view()),
    du.path('backups/<str:backup_id>', backups.BackupSkelView.as_view()),
    du.path('backups/<str:backup_id>/sections/<str:section>', backups.BackupSkelView.as_view()),

    du.path('shards', shards.ShardListSkelView.as_view()),
    du.path('shards/<str:shard_id>', shards.ShardSkelView.as_view()),
    du.path('shards/<str:shard_id>/sections/<str:section>', shards.ShardSkelView.as_view()),

    du.path('subclusters', subclusters.SubclusterListSkelView.as_view()),
    du.path('subclusters/<str:subcid_id>', subclusters.SubclusterSkelView.as_view()),
    du.path('subclusters/<str:subcid_id>/sections/<str:section>', subclusters.SubclusterSkelView.as_view()),

    du.path('valid_resources', valid_resources.ValidResourceListSkelView.as_view()),
    du.path('valid_resources/<str:id>', valid_resources.ValidResourceSkelView.as_view()),
    du.path('valid_resources/<str:id>/sections/<str:section>', valid_resources.ValidResourceSkelView.as_view()),

    du.path('ajax/clouds', clouds.CloudListView.as_view()),
    du.path('ajax/clouds/<str:cloud_id>/sections/<str:section>', clouds.CloudSectionView.as_view()),

    du.path('versions', versions.VersionListSkelView.as_view()),

    du.path('default_versions', default_versions.DefaultVersionListSkelView.as_view()),

    du.path('ajax/hosts', hosts.HostListView.as_view()),
    du.path('ajax/hosts/<str:fqdn>/sections/<str:section>', hosts.HostSectionView.as_view()),
    du.path('ajax/hosts/<str:fqdn>/blocks/<str:block>', hosts.HostBlockView.as_view()),

    du.path('ajax/clusters', clusters.ClusterListView.as_view()),
    du.path('ajax/clusters/<str:cluster_id>/sections/<str:section>', clusters.ClusterSectionView.as_view()),
    du.path('ajax/clusters/<str:cluster_id>/blocks/<str:block>', clusters.ClusterBlockView.as_view()),

    du.path('ajax/worker_tasks', worker_tasks.TaskListView.as_view()),
    du.path('ajax/worker_tasks/<str:task_id>/sections/<str:section>', worker_tasks.TaskSectionView.as_view()),
    du.path('ajax/worker_tasks/<str:task_id>/blocks/<str:block>', worker_tasks.TaskBlockView.as_view()),
    du.path('ajax/worker_tasks/dialogs/<str:action>', worker_tasks.TaskDialogView.as_view()),
    du.path('ajax/worker_tasks/actions/<str:action>', worker_tasks.TaskActionView.as_view()),

    du.path('ajax/valid_resources', valid_resources.ValidResourceListView.as_view()),
    du.path('ajax/valid_resources/<str:id>/sections/<str:section>', valid_resources.ValidResourceSectionView.as_view()),

    du.path('ajax/folders', folders.FolderListView.as_view()),
    du.path('ajax/folders/<str:folder_id>/sections/<str:section>', folders.FolderSectionView.as_view()),

    du.path('ajax/backups', backups.BackupListView.as_view()),
    du.path('ajax/backups/<str:backup_id>/sections/<str:section>', backups.BackupSectionView.as_view()),

    du.path('ajax/versions', versions.VersionListView.as_view()),

    du.path('ajax/default_versions', default_versions.DefaultVersionListView.as_view()),

    du.path('ajax/flavors', flavors.FlavorListView.as_view()),
    du.path('ajax/flavors/<str:id>/sections/<str:section>', flavors.FlavorSectionView.as_view()),

    du.path('ajax/subclusters', subclusters.SubclusterListView.as_view()),
    du.path('ajax/subclusters/<str:subcid_id>/sections/<str:section>', subclusters.SubclusterSectionView.as_view()),
    du.path('ajax/subclusters/<str:subcid_id>/blocks/<str:block>', subclusters.SubclusterBlockView.as_view()),

    du.path('ajax/shards', shards.ShardListView.as_view()),
    du.path('ajax/shards/<str:shard_id>/sections/<str:section>', shards.ShardSectionView.as_view()),
    du.path('ajax/shards/<str:shard_id>/blocks/<str:block>', shards.ShardBlockView.as_view()),

    du.path('ajax/maintenance_tasks', maintenance_tasks.MaintenanceTaskListView.as_view()),
    du.path('ajax/maintenance_tasks/<str:task_id>/sections/<str:section>', maintenance_tasks.MaintenanceTaskSectionView.as_view()),
]

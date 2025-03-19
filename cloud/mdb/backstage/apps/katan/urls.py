import django.urls as du

import cloud.mdb.backstage.apps.katan.views.hosts as hosts
import cloud.mdb.backstage.apps.katan.views.clusters as clusters
import cloud.mdb.backstage.apps.katan.views.rollouts as rollouts
import cloud.mdb.backstage.apps.katan.views.schedules as schedules


urlpatterns = [
    du.path('clusters', clusters.ClusterListSkelView.as_view()),
    du.path('clusters/<str:cluster_id>', clusters.ClusterSkelView.as_view()),
    du.path('clusters/<str:cluster_id>/sections/<str:section>', clusters.ClusterSkelView.as_view()),

    du.path('hosts', hosts.HostListSkelView.as_view()),
    du.path('hosts/<str:fqdn>', hosts.HostSkelView.as_view()),
    du.path('hosts/<str:fqdn>/sections/<str:section>', hosts.HostSkelView.as_view()),

    du.path('rollouts', rollouts.RolloutListSkelView.as_view()),
    du.path('rollouts/<str:rollout_id>', rollouts.RolloutSkelView.as_view()),
    du.path('rollouts/<str:rollout_id>/sections/<str:section>', rollouts.RolloutSkelView.as_view()),

    du.path('schedules', schedules.ScheduleListSkelView.as_view()),
    du.path('schedules/<str:schedule_id>', schedules.ScheduleSkelView.as_view()),
    du.path('schedules/<str:schedule_id>/sections/<str:section>', schedules.ScheduleSkelView.as_view()),

    du.path('ajax/clusters', clusters.ClusterListView.as_view()),
    du.path('ajax/clusters/<str:cluster_id>/sections/<str:section>', clusters.ClusterSectionView.as_view()),
    du.path('ajax/clusters/<str:cluster_id>/blocks/<str:block>', clusters.ClusterBlockView.as_view()),

    du.path('ajax/hosts', hosts.HostListView.as_view()),
    du.path('ajax/hosts/<str:fqdn>/sections/<str:section>', hosts.HostSectionView.as_view()),
    du.path('ajax/hosts/<str:fqdn>/blocks/<str:block>', hosts.HostBlockView.as_view()),

    du.path('ajax/rollouts', rollouts.RolloutListView.as_view()),
    du.path('ajax/rollouts/<str:rollout_id>/sections/<str:section>', rollouts.RolloutSectionView.as_view()),
    du.path('ajax/rollouts/<str:rollout_id>/blocks/<str:block>', rollouts.RolloutBlockView.as_view()),

    du.path('ajax/schedules', schedules.ScheduleListView.as_view()),
    du.path('ajax/schedules/<str:schedule_id>/sections/<str:section>', schedules.ScheduleSectionView.as_view()),
    du.path('ajax/schedules/<str:schedule_id>/blocks/<str:block>', schedules.ScheduleBlockView.as_view()),
]

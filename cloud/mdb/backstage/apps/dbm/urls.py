import django.urls as du

import cloud.mdb.backstage.apps.dbm.views.clusters as clusters
import cloud.mdb.backstage.apps.dbm.views.containers as containers
import cloud.mdb.backstage.apps.dbm.views.dom0_hosts as dom0_hosts
import cloud.mdb.backstage.apps.dbm.views.projects as projects
import cloud.mdb.backstage.apps.dbm.views.reserved_resources as reserved_resources
import cloud.mdb.backstage.apps.dbm.views.transfers as transfers


urlpatterns = [
    du.path('ajax/clusters', clusters.ClusterListView.as_view()),
    du.path('ajax/clusters/<str:name>/sections/<str:section>', clusters.ClusterSectionView.as_view()),

    du.path('clusters', clusters.ClusterListSkelView.as_view()),
    du.path('clusters/<str:name>', clusters.ClusterSkelView.as_view()),
    du.path('clusters/<str:name>/sections/<str:section>', clusters.ClusterSkelView.as_view()),

    du.path('containers', containers.ContainerListSkelView.as_view()),
    du.path('containers/<str:fqdn>', containers.ContainerSkelView.as_view()),
    du.path('containers/<str:fqdn>/sections/<str:section>', containers.ContainerSkelView.as_view()),

    du.path('dom0_hosts', dom0_hosts.Dom0HostListSkelView.as_view()),
    du.path('dom0_hosts/<str:fqdn>', dom0_hosts.Dom0HostSkelView.as_view()),
    du.path('dom0_hosts/<str:fqdn>/sections/<str:section>', dom0_hosts.Dom0HostSkelView.as_view()),

    du.path('projects', projects.ProjectListSkelView.as_view()),
    du.path('projects/<str:name>', projects.ProjectSkelView.as_view()),
    du.path('projects/<str:name>/sections/<str:section>', projects.ProjectSkelView.as_view()),

    du.path('reserved_resources', reserved_resources.ReservedResourceListSkelView.as_view()),

    du.path('transfers', transfers.TransferListSkelView.as_view()),
    du.path('transfers/<str:id>', transfers.TransferSkelView.as_view()),
    du.path('transfers/<str:id>/sections/<str:section>', transfers.TransferSkelView.as_view()),

    du.path('ajax/containers', containers.ContainerListView.as_view()),
    du.path('ajax/containers/<str:fqdn>/sections/<str:section>', containers.ContainerSectionView.as_view()),

    du.path('ajax/dom0_hosts', dom0_hosts.Dom0HostListView.as_view()),
    du.path('ajax/dom0_hosts/<str:fqdn>/sections/<str:section>', dom0_hosts.Dom0HostSectionView.as_view()),
    du.path('ajax/dom0_hosts/<str:fqdn>/blocks/<str:block>', dom0_hosts.Dom0HostBlockView.as_view()),

    du.path('ajax/projects', projects.ProjectListView.as_view()),
    du.path('ajax/projects/<str:name>/sections/<str:section>', projects.ProjectSectionView.as_view()),
    du.path('ajax/projects/<str:name>/blocks/<str:block>', projects.ProjectBlockView.as_view()),

    du.path('ajax/reserved_resources', reserved_resources.ReservedResourceListView.as_view()),

    du.path('ajax/transfers', transfers.TransferListView.as_view()),
    du.path('ajax/transfers/<str:id>/sections/<str:section>', transfers.TransferSectionView.as_view()),
]

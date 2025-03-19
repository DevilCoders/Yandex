import django.urls as du

import cloud.mdb.backstage.apps.deploy.views.shipments as shipments
import cloud.mdb.backstage.apps.deploy.views.job_results as job_results
import cloud.mdb.backstage.apps.deploy.views.commands as commands
import cloud.mdb.backstage.apps.deploy.views.groups as groups
import cloud.mdb.backstage.apps.deploy.views.masters as masters
import cloud.mdb.backstage.apps.deploy.views.minions as minions
import cloud.mdb.backstage.apps.deploy.views.shipment_commands as shipment_commands


urlpatterns = [
    du.path('shipments', shipments.ShipmentListSkelView.as_view()),
    du.path('shipments/<str:shipment_id>', shipments.ShipmentSkelView.as_view()),
    du.path('shipments/<str:shipment_id>/sections/<str:section>', shipments.ShipmentSkelView.as_view()),

    du.path('job_results', job_results.JobResultListSkelView.as_view()),
    du.path('job_results/<str:job_result_id>', job_results.JobResultSkelView.as_view()),
    du.path('job_results/<str:job_result_id>/sections/<str:section>', job_results.JobResultSkelView.as_view()),

    du.path('commands', commands.CommandListSkelView.as_view()),
    du.path('commands/<str:command_id>', commands.CommandSkelView.as_view()),
    du.path('commands/<str:command_id>/sections/<str:section>', commands.CommandSkelView.as_view()),

    du.path('groups', groups.GroupListSkelView.as_view()),
    du.path('groups/<str:group_id>', groups.GroupSkelView.as_view()),
    du.path('groups/<str:group_id>/sections/<str:section>', groups.GroupSkelView.as_view()),

    du.path('masters', masters.MasterListSkelView.as_view()),
    du.path('masters/<str:master_id>', masters.MasterSkelView.as_view()),
    du.path('masters/<str:master_id>/sections/<str:section>', masters.MasterSkelView.as_view()),

    du.path('minions', minions.MinionListSkelView.as_view()),
    du.path('minions/<str:minion_id>', minions.MinionSkelView.as_view()),
    du.path('minions/<str:minion_id>/sections/<str:section>', minions.MinionSkelView.as_view()),

    du.path('shipment_commands', shipment_commands.ShipmentCommandListSkelView.as_view()),
    du.path('shipment_commands/<str:shipment_command_id>', shipment_commands.ShipmentCommandSkelView.as_view()),
    du.path('shipment_commands/<str:shipment_command_id>/sections/<str:section>', shipment_commands.ShipmentCommandSkelView.as_view()),

    du.path('ajax/commands', commands.CommandListView.as_view()),
    du.path('ajax/commands/<str:command_id>/sections/<str:section>', commands.CommandSectionView.as_view()),
    du.path('ajax/commands/<str:command_id>/blocks/<str:block>', commands.CommandBlockView.as_view()),

    du.path('ajax/groups', groups.GroupListView.as_view()),
    du.path('ajax/groups/<str:group_id>/sections/<str:section>', groups.GroupSectionView.as_view()),

    du.path('ajax/masters', masters.MasterListView.as_view()),
    du.path('ajax/masters/<str:master_id>/sections/<str:section>', masters.MasterSectionView.as_view()),

    du.path('ajax/minions', minions.MinionListView.as_view()),
    du.path('ajax/minions/<str:minion_id>/sections/<str:section>', minions.MinionSectionView.as_view()),

    du.path('ajax/shipment_commands', shipment_commands.ShipmentCommandListView.as_view()),
    du.path('ajax/shipment_commands/<str:shipment_command_id>/sections/<str:section>', shipment_commands.ShipmentCommandSectionView.as_view()),
    du.path('ajax/shipment_commands/<str:shipment_command_id>/blocks/<str:block>', shipment_commands.ShipmentCommandBlockView.as_view()),

    du.path('ajax/shipments', shipments.ShipmentListView.as_view()),
    du.path('ajax/shipments/<str:shipment_id>/sections/<str:section>', shipments.ShipmentSectionView.as_view()),
    du.path('ajax/shipments/<str:shipment_id>/blocks/<str:block>', shipments.ShipmentBlockView.as_view()),

    du.path('ajax/job_results', job_results.JobResultListView.as_view()),
    du.path('ajax/job_results/<str:job_result_id>/sections/<str:section>', job_results.JobResultSectionView.as_view()),
    du.path('ajax/job_results/<str:job_result_id>/blocks/<str:block>', job_results.JobResultBlockView.as_view()),
]

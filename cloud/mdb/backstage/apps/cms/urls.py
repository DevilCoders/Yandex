import django.urls as du

import cloud.mdb.backstage.apps.cms.views.decisions as decisions
import cloud.mdb.backstage.apps.cms.views.instance_operations as instance_operations


urlpatterns = [
    du.path('decisions', decisions.DecisionListSkelView.as_view()),
    du.path('decisions/<int:decision_id>', decisions.DecisionSkelView.as_view()),
    du.path('decisions/<int:decision_id>/sections/<str:section>', decisions.DecisionSkelView.as_view()),

    du.path('instance_operations', instance_operations.InstanceOperationListSkelView.as_view()),
    du.path('instance_operations/<uuid:instance_operation_id>', instance_operations.InstanceOperationSkelView.as_view()),
    du.path('instance_operations/<uuid:instance_operation_id>/sections/<str:section>', instance_operations.InstanceOperationSkelView.as_view()),

    du.path('ajax/decisions', decisions.DecisionListView.as_view()),
    du.path('ajax/decisions/<int:decision_id>/sections/common', decisions.DecisionSectionView.as_view()),
    du.path('ajax/decisions/dialogs/<str:action>', decisions.DecisionDialogView.as_view()),
    du.path('ajax/decisions/actions/<str:action>', decisions.DecisionActionView.as_view()),

    du.path('ajax/instance_operations', instance_operations.InstanceOperationListView.as_view()),
    du.path('ajax/instance_operations/dialogs/<str:action>', instance_operations.InstanceOperationDialogView.as_view()),
    du.path('ajax/instance_operations/actions/<str:action>', instance_operations.InstanceOperationActionView.as_view()),
    du.path('ajax/instance_operations/<uuid:instance_operation_id>/sections/<str:section>', instance_operations.InstanceOperationSectionView.as_view()),
]

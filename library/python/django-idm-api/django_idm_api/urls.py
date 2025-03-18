from __future__ import unicode_literals

from django_idm_api import views, membership_views
from django_idm_api.compat import url

app_name = 'django_idm_api'

urlpatterns = (
    url(r'^info/$', views.info, name='info'),
    url(r'^get-all-roles/$', views.get_all_roles, name='get-all-roles'),
    url(r'^get-roles/$', views.get_roles, name='get-roles'),
    url(r'^add-role/$', views.add_role, name='add-role'),
    url(r'^remove-role/$', views.remove_role, name='remove-role'),

    url(r'^get-memberships/$', membership_views.get_memberships, name='get-memberships'),
    url(r'^add-membership/$', membership_views.add_membership, name='add-membership'),
    url(r'^add-batch-memberships/$', membership_views.add_batch_memberships, name='add-batch-memberships'),
    url(r'^remove-membership/$', membership_views.remove_membership, name='remove-membership'),
    url(r'^remove-batch-memberships/$', membership_views.remove_batch_memberships, name='remove-batch-memberships'),
)

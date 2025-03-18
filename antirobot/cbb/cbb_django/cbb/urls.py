from django.urls import path
from antirobot.cbb.cbb_django.cbb.views import admin, api, antiddos

app_name = "antirobot.cbb.cbb_django.cbb"

admin_urlpatterns = [
    path("",       admin.index,  name="admin.index"),
    path("groups", admin.groups, name="admin.groups"),
    path("search", admin.search, name="admin.search"),
    path("status", admin.status, name="admin.status"),
    path("staff/<str:login>",  admin.staff, name="admin.staff"),
    path("service/add", admin.add_service, name="admin.add_service"),
    path("service/<str:service>", admin.groups, name="admin.service"),
    path("tag/<str:tag>", admin.groups, name="admin.tag"),

    path("groups/add",                        admin.add_group,         name="admin.add_group"),
    path("groups/<slug:group_id>",            admin.view_group_blocks, name="admin.view_group_blocks"),
    path("groups/<slug:group_id>/delete",     admin.delete_group,      name="admin.delete_group"),
    path("groups/<slug:group_id>/edit",       admin.edit_group,        name="admin.edit_group"),
    path("groups/<slug:group_id>/view",       admin.view_group,        name="admin.view_group"),
    path("groups/<slug:group_id>/add_range",  admin.add_range,         name="admin.add_range"),
    path("groups/<slug:group_id>/view_range", admin.view_range,        name="admin.view_range"),
    path("groups/<slug:group_id>/edit_range", admin.edit_range,        name="admin.edit_range"),

    path("edit_range",                        admin.edit_range,        name="admin.edit_range"),
    path("view_range",                        admin.view_range,        name="admin.view_range"),
]

ajax_urlpatterns = [
    path("ajax/ajax_antiddos_check", antiddos.ajax_antiddos_check, name="antiddos.ajax_antiddos_check"),
    path("ajax/ajax_get_random_requests", antiddos.ajax_get_random_requests, name="antiddos.ajax_get_random_requests"),
]

api_urlpatterns = [
    # check_flag и get_range отсутствуют, т.к. они в cbb_fast
    path("api/v1/set_range",     api.set_range,     name="api.v1.set_range"),
    path("api/v1/set_ranges",    api.set_ranges,    name="api.v1.set_ranges"),
    path("api/v1/get_netblock",  api.get_netblock,  name="api.v1.get_netblock"),
    path("api/v1/set_netblock",  api.set_netblock,  name="api.v1.set_netblock"),
    path("api/v1/get_netexcept", api.get_netexcept, name="api.v1.get_netexcept"),
    path("api/v1/set_netexcept", api.set_netexcept, name="api.v1.set_netexcept"),
]

urlpatterns = admin_urlpatterns + api_urlpatterns + ajax_urlpatterns

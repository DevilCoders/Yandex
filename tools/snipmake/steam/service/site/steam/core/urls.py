# -*- coding: utf-8 -*-

from django.conf.urls import patterns, url

from core.models import MAX_STORAGE_ID
from core.settings import SEGMENTATION_ENABLED

patterns_args = [
    'core.views',
    url(r'^$', 'index', name='index'),
    url(r'^index.html$', 'index', name='index'),
    url(r'^alive/$', 'alive', name='alive'),
    url(r'^health/$', 'health', name='health'),

    url(r'^help/$', 'help', name='help'),

    url(r'^raw_files/$', 'raw_files', {'page': 0}, name='raw_files_default'),
    url(r'^raw_files/(?P<page>\d+)/$', 'raw_files', name='raw_files'),
    url(r'^get_raw_file/(?P<storage_id>[A-F0-9]{%d})/$' % MAX_STORAGE_ID, 'get_raw_file', name='get_raw_file'),
    url(r'^waiting_users/$', 'waiting_users', {'page': 0}, name='waiting_users_default'),
    url(r'^waiting_users/(?P<page>\d+)/$', 'waiting_users', name='waiting_users'),
    url(r'^roles/$', 'roles', {'page': 0}, name='roles_default'),
    url(r'^roles/(?P<page>\d+)/$', 'roles', name='roles'),

    url(r'^set_role/$', 'set_role', {'page': 0}, name='set_role_default'),
    url(r'^set_role/(?P<page>\d+)$', 'set_role', name='set_role'),

    url(r'^request_permission/$', 'request_permission', name='request_permission'),
    url(r'^add_user/$', 'add_user', name='add_user'),

    url(r'^querybins/$', 'querybins', {'page': 0}, name='querybins_default'),
    url(r'^querybins/(?P<page>\d+)/$', 'querybins', name='querybins'),
    url(r'^querybins/add/(?P<page>\d+)/$', 'add_querybin', name='add_querybin'),
    url(r'^querybins/delete/(?P<qb_id>\d+)/(?P<page>\d+)/$', 'delete_querybin', name='delete_querybin'),

    url(r'^snippetpools/$', 'snippetpools', {'page': 0}, name='snippetpools_default'),
    url(r'^snippetpools/(?P<page>\d+)/$', 'snippetpools', name='snippetpools'),
    url(r'^snippetpools/add/(?P<page>\d+)/$', 'add_snippetpool', name='add_snippetpool'),
    url(r'^snippetpools/add_two/$', 'add_two_snippetpools', name='add_two_snippetpools'),
    url(r'^snippetpools/delete/(?P<storage_id>[A-F0-9]{%d})/(?P<page>\d+)/$' % MAX_STORAGE_ID, 'delete_snippetpool', name='delete_snippetpool'),

    url(r'^snippets/(?P<storage_id>[A-F0-9]{%d})/$' % MAX_STORAGE_ID, 'snippets', {'page': 0}, name='snippets_default'),
    url(r'^snippets/(?P<storage_id>[A-F0-9]{%d})/(?P<page>\d+)/$' % MAX_STORAGE_ID, 'snippets', name='snippets'),

    url(r'^taskpools/(?P<tab>ang)/$', 'taskpools', {'page': 0}, name='taskpools_default'),
    url(r'^taskpools/$', 'taskpools', {'tab': '', 'page': 0}, name='taskpools_default'),
    url(r'^taskpools/(?P<tab>ang)/(?P<page>\d+)/$', 'taskpools', name='taskpools'),
    url(r'^taskpools/(?P<page>\d+)/$', 'taskpools', {'tab': ''}, name='taskpools'),
    url(r'^taskpools/add/$', 'add_taskpool', name='add_taskpool'),
    url(r'^taskpools/start/(?P<taskpool_id>\d+)/$', 'start_taskpool', name='start_taskpool'),
    url(r'^taskpools/(?P<action>(disable)|(finish))/(?P<taskpool_id>\d+)/(?P<page>\d+)/$', 'stop_taskpool', name='stop_taskpool'),
    url(r'^taskpools/edit/(?P<taskpool_id>\d+)/$', 'edit_taskpool', name='edit_taskpool'),
    url(r'^taskpools/delete/(?P<taskpool_id>\d+)/(?P<page>\d+)/$', 'delete_taskpool', name='delete_taskpool'),
    url(r'^taskpools/export/(?P<taskpool_id>\d+)/(?P<page>\d+)/$', 'export_taskpool', name='export_taskpool'),
    url(r'^taskpools/view/(?P<taskpool_id>\d+)/(?P<order>(alphabet)|(random))/$', 'view_taskpool', {'page': 0}, name='view_taskpool_default'),
    url(r'^taskpools/view/(?P<taskpool_id>\d+)/(?P<order>(alphabet)|(random))/(?P<page>\d+)/$', 'view_taskpool', name='view_taskpool'),
    url(r'^taskpools/monitor/(?P<taskpool_id>\d+)/$', 'monitor_taskpool', name='monitor_taskpool'),

    url(r'^tasks/(?P<taskpool_id>\d+)/(?P<status>(all)|(inspection))/(?P<order>(alphabet)|(random))/$', 'tasks', {'page': 0}, name='tasks_default'),
    url(r'^tasks/(?P<taskpool_id>\d+)/(?P<status>(all)|(inspection))/(?P<order>(alphabet)|(random))/(?P<page>\d+)/$', 'tasks', name='tasks'),

    url(r'^tasks/delete/(?P<task_id>\d+)/(?P<order>(alphabet)|(random))/(?P<page>\d+)/$', 'delete_task', name='delete_task'),

    url(r'^usertasks/(?P<tab>(current)|(finished)|(checked)|(questions))/$', 'usertasks', {'page': 0}, name='usertasks_default'),
    url(r'^usertasks/(?P<tab>(current)|(finished)|(checked)|(questions))/(?P<page>\d+)/$', 'usertasks', name='usertasks'),
    url(r'^usertasks/user_take/(?P<taskpool_id>\d+)/(?P<page>\d+)/$', 'take_user_tasks', name='take_user_tasks'),

    url(r'^usertasks/available/$', 'available_usertasks', {'page': 0}, name='available_usertasks_default'),
    url(r'^usertasks/available/(?P<page>\d+)/$', 'available_usertasks', name='available_usertasks'),
    url(r'^usertasks/packs/$', 'usertask_packs', {'page': 0}, name='usertask_packs_default'),
    url(r'^usertasks/packs/(?P<page>\d+)/$', 'usertask_packs', name='usertask_packs'),
    url(r'^usertasks/aadmin_take/(?P<task_id>\d+)/$', 'take_aadmin_task', name='take_aadmin_task'),
    url(r'^usertasks/take_aadmin_task_batch/(?P<action>take)/(?P<page>\d+)/$', 'take_aadmin_task_batch', name='take_aadmin_task_batch'),
    url(r'^usertasks/take_aadmin_task_batch/(?P<action>assign)/(?P<taskpool_id>\d+)/(?P<status>(all)|(inspection))/(?P<order>(alphabet)|(random))/(?P<page>\d+)/$', 'take_aadmin_task_batch', name='take_aadmin_task_batch'),

    url(r'^monitor/$', 'monitor', {'page': 0}, name='monitor_default'),
    url(r'^monitor/(?P<page>\d+)/$', 'monitor', name='monitor'),
    url(r'^ajax/monitor/$', 'monitor_statuses', {'page': 0}, name='monitor_statuses_default'),
    url(r'^ajax/monitor/(?P<page>\d+)/$', 'monitor_statuses', name='monitor_statuses'),
    url(r'^ajax/stop/(?P<celery_id>[a-f0-9]{8}-(?:[a-f0-9]{4}-){3}[a-f0-9]{12})/$', 'stop_bg_task', name='stop_bg_task'),

    url(r'^ajax/fileupload/$', 'ajax_fileupload', name='ajax_fileupload'),
    url(r'^ajax/filedelete/$', 'ajax_filedelete', name='ajax_filedelete'),

    url(r'^estimation/(?P<est_id>\d+)/(?:(?P<exp_criterion>(informativity)|(content_richness)|(readability)|(media_content))/)?$', 'estimation', name='estimation'),
    url(r'^estimation/check/(?P<est_id>\d+)/$', 'estimation_check', name='estimation_check'),
    url(r'^estimation/process_errors/(?P<est_id>\d+)/$', 'process_errors', name='process_errors'),
    url(r'^corrections/(?P<tab>(all)|(significant))/(?P<task_id>\d+)/$', 'corrections', name='corrections'),

    url(r'^send_email/(?P<est_id>\d+)/$', 'send_email', name='send_email'),

    url(r'^statistics/(?P<taskpool_id>\d+)/$', 'statistics', {'criterion': None, 'value_name': None}, name='statistics_default'),
    url(r'^statistics/(?P<taskpool_id>\d+)/(?P<criterion>(informativity)|(content_richness)|(readability)|(media_content))_(?P<value_name>(left)|(both)|(right))/$', 'statistics', name='statistics'),

    url(r'^ajax/statistics/(?P<taskpool_id>\d+)/(?P<yandex_uid>\d+)/$', 'user_statistics', {'criterion': None, 'value_name': None}, name='user_statistics_default'),
    url(r'^ajax/statistics/(?P<taskpool_id>\d+)/(?P<criterion>(informativity)|(content_richness)|(readability)|(media_content))_(?P<value_name>(left)|(both)|(right))/(?P<yandex_uid>\d+)/$', 'user_statistics', name='user_statistics'),

    url(r'^yauth/$', 'yauth', name='yauth'),

    url(r'^user/(?P<login>[\w.-]+)/$', 'user_info', name='user_info'),
    url(r'^clear_estimations/$', 'clear_estimations', name='clear_estimations'),
    url(r'^assign_pack_back/(?P<pack_id>\d+)/$', 'assign_pack_back', name='assign_pack_back'),
    url(r'^usertasks/take_task_by_as/(?P<assessor_id>\d+)/$', 'take_task_by_as', name='take_task_by_as'),

    url(r'^ajax/notifications/$', 'notifications', name='notifications'),
    url(r'^ajax/close_notification/(?P<notif_id>\d+)/$', 'close_notification', name='close_notification'),

    url(r'^estimation/statistics/$', 'est_statistics', {'page': 0}, name='est_statistics_default'),
    url(r'^estimation/statistics/(?P<page>\d+)/$', 'est_statistics', name='est_statistics'),
    url(r'^estimation/statistics/time/$', 'time_statistics', name='time_statistics'),
    url(r'^estimation/statistics/assessors/$', 'assessor_statistics', name='assessor_statistics'),
]

if SEGMENTATION_ENABLED:
    patterns_args += [
        url(r'^page_segmentation/(?P<est_id>\d+)/$', 'page_segmentation', name='page_segmentation'),
        url(r'^page_segmentation/list/(?P<page>\d+)/$', 'segmentation_list', name='segmentation_list'),
        url(r'^page_segmentation/list/$', 'segmentation_list', {'page': 0}, name='segmentation_list_default'),
        url(r'^page_segmentation/view/(?P<docid>\d+)/(?P<login>[\w.-]+)/$', 'segmentation_view', name='segmentation_view'),
        url(r'^page_segmentation/diff/(?P<docid>\d+)/(?P<login1>[\w.-]+)/(?P<login2>[\w.-]+)/$', 'segmentation_diff', name='segmentation_diff'),
        url(r'^page_segmentation/delete/(?P<docid>\d+)/(?P<login>[\w.-]+)/$', 'segmentation_delete', name='segmentation_delete'),

        url(r'^page_segmentation/tasks/(?P<page>\d+)/$', 'segmentation_tasks', name='segmentation_tasks'),
        url(r'^page_segmentation/tasks/$', 'segmentation_tasks', {'page': 0}, name='segmentation_tasks_default'),
        url(r'^page_segmentation/take/$', 'segmentation_take', name='segmentation_take'),
    ]

urlpatterns = patterns(*patterns_args)

# coding: utf-8

from .new_org import create_new_organization, sync_new_organizations
from .sync_data import sync_dir_data_changes, ProcessChangeEventForOrgTask, ensure_service_ready

from wiki.celery_apps import app
app.tasks.register(ProcessChangeEventForOrgTask)


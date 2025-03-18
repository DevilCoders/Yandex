# -*- coding: utf-8 -*-

import json
import os
import tempfile

from django.core.exceptions import ObjectDoesNotExist
from django.utils import timezone

from django.utils.translation import ugettext_noop as _

from core.settings import TEMP_ROOT
from core.actions.common import jsonify
from core.actions.storage import Storage
from core.hard import downloader
from core.hard.loghandlers import SteamLogger

from core.models import BackgroundTask, QueryBin, Country
from core.tasks import bg_download_serp, bg_store_snippetpool


def process_querybin(request):
    country = request.POST['country']
    filename = os.path.join(TEMP_ROOT, request.POST['tmp_file'])
    qb_file = open(filename)
    queries = []
    for line in qb_file:
        line = line.replace('\n', '')
        if line:
            tokens = line.split('\t')
            if tokens[0]:
                try:
                    region = int(tokens[1])
                except (IndexError, ValueError):
                    region = Country.REGION_CHOICES[country]
                if len(tokens) > 1:
                    tokens[1] = str(region)
                else:
                    tokens.append(str(region))
                queries.append('\t'.join(tokens[:3]))
    count = len(queries)
    content = '\n'.join(queries)
    tmp_name = save_to_temp(content)
    qb_file.close()
    os.remove(filename)
    storage_id = Storage.store_raw_file(tmp_name)
    if not storage_id:
        return False
    user = request.yauser.core_user
    query_bin = QueryBin(user=user, title=request.POST['title'],
                         upload_time=timezone.now(), country=country,
                         count=count, storage_id=storage_id)
    query_bin.save()
    os.remove(tmp_name)
    return True


def create_snippetpool_task(request, tmp_name, title, default_tpl, json_pool):
    celery_id = str(bg_store_snippetpool.delay(tmp_name, title, default_tpl,
                                               json_pool))
    bg_task = BackgroundTask(
        celery_id=celery_id,
        title=title,
        task_type=BackgroundTask.Type.UPLOAD,
        status=_('PLANNED'),
        start_time=timezone.now(),
        user=request.yauser.core_user,
        storage_id=''
    )
    bg_task.save()


def process_snippetpool(request, json_pool):
    title = request.POST['title']
    default_tpl = request.POST['default_tpl']
    tmp_name = os.path.join(TEMP_ROOT, request.POST['tmp_pool_file'])
    create_snippetpool_task(request, tmp_name, title, default_tpl, json_pool)


def process_two_snippetpools(request):
    title_prefix = request.POST['title_prefix']
    base_title = title_prefix + request.POST['base_title_suffix']
    exp_title = title_prefix + request.POST['exp_title_suffix']
    base_default_tpl = request.POST['base_default_tpl']
    exp_default_tpl = request.POST['exp_default_tpl']

    two_pools_filename = os.path.join(TEMP_ROOT,
                                      request.POST['tmp_two_pools_file'])
    two_pools_file = open(two_pools_filename)
    base_handle, base_tmp_name = tempfile.mkstemp(dir=TEMP_ROOT)
    exp_handle, exp_tmp_name = tempfile.mkstemp(dir=TEMP_ROOT)
    for line in two_pools_file:
        line = line.rstrip()
        type_for_log = 'JSONIFY_TWO_SNIP_POOLS_LOAD_ERROR'
        two_pools_json = jsonify(line, type_for_log)
        if not (isinstance(two_pools_json, list) and len(two_pools_json) == 2):
            SteamLogger.error(
                'Json fails on load:"%(json_str)s" error:"%(err)s"',
                json_str=line, err='not list or len != 2',
                type=type_for_log
            )
            continue
        os.write(base_handle, json.dumps(two_pools_json[0], encoding='utf-8',
                                         ensure_ascii=False).encode('utf-8'))
        os.write(base_handle, '\n')
        os.write(exp_handle, json.dumps(two_pools_json[1], encoding='utf-8',
                                        ensure_ascii=False).encode('utf-8'))
        os.write(exp_handle, '\n')
    os.close(exp_handle)
    os.close(base_handle)
    two_pools_file.close()

    create_snippetpool_task(request, base_tmp_name, base_title,
                            base_default_tpl, True)
    create_snippetpool_task(request, exp_tmp_name, exp_title,
                            exp_default_tpl, True)


def download_serp(request):
    title = request.POST['title']
    default_tpl = request.POST['default_tpl']
    host = hamster_url(request.POST['host'])
    if host == 'Choose':
        host = request.POST['url']
    login = '_'.join(('steam', request.yauser.core_user.login))
    params_string = request.POST['cgi_params']
    dl = downloader.Downloader(login=login, host=host,
                               params_string=params_string)
    try:
        qb_object = QueryBin.objects.get(pk=request.POST['querybin'])
        querybin = Storage.get_raw_file(qb_object.storage_id)
    except KeyError, ObjectDoesNotExist:
        querybin = ''
    for query in querybin.split('\n'):
        if query:
            query_dict = dict(zip(('text', 'region', 'url'),
                                  query.split('\t')))
            dl.add_query(downloader.Query(query_dict))
    celery_id = str(bg_download_serp.apply_async(args=[dl, title, default_tpl], queue='serp'))
    bg_task = BackgroundTask(
        celery_id=celery_id,
        title=title,
        task_type=BackgroundTask.Type.SERP,
        status=_('PLANNED'),
        start_time=timezone.now(),
        user=request.yauser.core_user,
        storage_id=''
    )
    bg_task.save()


def hamster_url(host):
    return '.'.join(('hamster', host))


def save_to_temp(content):
    handle, tmp_name = tempfile.mkstemp(dir=TEMP_ROOT)
    os.write(handle, content)
    os.close(handle)
    return tmp_name

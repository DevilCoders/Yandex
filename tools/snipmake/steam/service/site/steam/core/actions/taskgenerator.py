# -*- coding: utf-8 -*-

import json
import random

from django.utils import timezone
from django.shortcuts import get_object_or_404
from django.db.models import Q

from core.hard.loghandlers import SteamLogger
from core.actions import snipextraparser, sniptpl
from core.actions.snipextraparser import JsonFields
from core.actions.storage import Storage
from core.models import SnippetPool, TaskPool, Task, TPCountry, Snippet
from core.settings import DEFAULT_TASKPOOL_LIFETIME, HIDE_BOLD_RE, BULK_CREATE_BATCH_SIZE


def get_taskpool_obj(request, snippet_pool_obj_1, snippet_pool_obj_2):
    cur_time = timezone.now()
    # taskpool.count will set below
    taskpool = TaskPool(title=request.POST['title'],
                        create_time=cur_time,
                        kind=request.POST['kind'],
                        status=TaskPool.Status.DISABLED,
                        priority=request.POST['priority'],
                        overlap=request.POST['overlap'],
                        ang_taskset_id='',
                        first_pool=snippet_pool_obj_1,
                        first_pool_tpl=request.POST['first_pool_tpl'],
                        second_pool=snippet_pool_obj_2,
                        second_pool_tpl=request.POST['second_pool_tpl'],
                        count=0,
                        deadline=(timezone.localtime(cur_time).date() +
                                  DEFAULT_TASKPOOL_LIFETIME),
                        user=request.yauser.core_user,
                        kind_pool=request.POST['kind_pool'])
    return taskpool


def get_snippet_key(snippet):
    return ''.join((snippet.Query, snippet.Url))


def get_snippet_pk(snip_pool_id, snip_id):
    return '_'.join((snip_pool_id, str(snip_id)))


def get_snippet_obj(snippet_proto, snippet_id):
    #Region is not required for RCA, but cast to int. Default value for this is 0
    data = {'query': snippet_proto.Query.decode('utf-8'),
            'url': snippet_proto.Url.decode('utf-8'),
            JsonFields.HILITEDURL: snippet_proto.HilitedUrl.decode('utf-8'),
            'region': int(snippet_proto.Region) if snippet_proto.Region != '' else 0,
            'title': snippet_proto.TitleText.decode('utf-8'),
            JsonFields.HEADLINE: snippet_proto.Headline.decode('utf-8'),
            JsonFields.HEADLINE_SRC: snippet_proto.HeadlineSrc.decode('utf-8'),
            'text': [fr.Text.decode('utf-8')
                     for fr in snippet_proto.Fragments]}
    if snippet_proto.Lines != '':
        data['lines'] = int(snippet_proto.Lines)

    data['extra'] = {}
    if snippet_proto.ExtraInfo != '':
        extra_info = snippet_proto.ExtraInfo.decode('utf-8')
        try:
            extra_info = json.loads(extra_info, encoding='utf-8')
        except (TypeError, ValueError) as exc:
            SteamLogger.error(
                'Json fails on load snippet:%(snippet_id)s extra:%(extra)s error: %(err)s',
                snippet_id=snippet_id, extra=extra_info, err=exc,
                type='SNIPPET_EXTRA_LOAD_ERROR'
            )
        else:
            data['extra'] = snipextraparser.parse(extra_info)

            if JsonFields.URLMENU in data['extra']:
                data[JsonFields.URLMENU] = data['extra'][JsonFields.URLMENU]
                del data['extra'][JsonFields.URLMENU]
            if JsonFields.IMG in data['extra']:
                data[JsonFields.IMG] = data['extra'][JsonFields.IMG]
            if JsonFields.LINK_ATTRS in data['extra']:
                data[JsonFields.LINK_ATTRS] = data['extra'][JsonFields.LINK_ATTRS]

    snippet = Snippet(
        snippet_id=snippet_id,
        data=data
    )
    return snippet


def is_snippets_equal(snip_1, snip_2, ignore_bold):
    # TODO: maybe compare rendered templates
    headline_src_1 = snip_1.data.get(JsonFields.HEADLINE_SRC, '')
    headline_src_2 = snip_2.data.get(JsonFields.HEADLINE_SRC, '')
    if headline_src_1 != headline_src_2:
        return False
    title_1, headline_1, text_1 = snip_1.data['title'], \
        snip_1.data.get('headline', ''), ''.join(snip_1.data['text'])
    mediacontent_1 = ''
    if JsonFields.IMG in snip_1.data:
        mediacontent_1 = snip_1.data[JsonFields.IMG]
    title_2, headline_2, text_2 = snip_2.data['title'], \
        snip_2.data.get('headline', ''), ''.join(snip_2.data['text'])
    mediacontent_2 = ''
    if JsonFields.IMG in snip_2.data:
        mediacontent_2 = snip_2.data[JsonFields.IMG]
    if ignore_bold:
        title_1 = HIDE_BOLD_RE.sub('', title_1)
        headline_1 = HIDE_BOLD_RE.sub('', headline_1)
        text_1 = HIDE_BOLD_RE.sub('', text_1)
        title_2 = HIDE_BOLD_RE.sub('', title_2)
        headline_2 = HIDE_BOLD_RE.sub('', headline_2)
        text_2 = HIDE_BOLD_RE.sub('', text_2)
    return (title_1 == title_2 and headline_1 == headline_2
            and text_1 == text_2 and mediacontent_1 == mediacontent_2)


def wrong_template(snippet, tpl):
    return not (
        tpl == SnippetPool.Template.ANY or
        sniptpl.get_tpl_class(snippet.HeadlineSrc).tpl_type == tpl
    )


class TemplatedSnippetIterator:
    def __init__(self, snip_pool_id, tpl):
        self.it = Storage.snippet_iterator(snip_pool_id)
        self.tpl = tpl

    def next(self):
        snip = self.it.next()
        while wrong_template(snip, self.tpl):
            snip = self.it.next()
        return snip

    def snip_id(self):
        return self.it.snip_id()


def task_snippets_generator(request, snippet_pool_1_id, snippet_pool_2_id,
                            ignore_bold):
    it_1 = TemplatedSnippetIterator(snippet_pool_1_id,
                                    request.POST['first_pool_tpl'])
    it_2 = TemplatedSnippetIterator(snippet_pool_2_id,
                                    request.POST['second_pool_tpl'])
    try:
        snippet_1 = it_1.next()
        snippet_2 = it_2.next()
        key_1 = get_snippet_key(snippet_1)
        key_2 = get_snippet_key(snippet_2)
        while True:
            if key_1 == key_2:
                snippet_1_obj = get_snippet_obj(
                    snippet_1,
                    get_snippet_pk(snippet_pool_1_id, it_1.snip_id()))
                snippet_2_obj = get_snippet_obj(
                    snippet_2,
                    get_snippet_pk(snippet_pool_2_id, it_2.snip_id()))
                if not is_snippets_equal(snippet_1_obj, snippet_2_obj,
                                         ignore_bold):
                    yield (snippet_1_obj, snippet_2_obj)
                snippet_1 = it_1.next()
                key_1 = get_snippet_key(snippet_1)
                snippet_2 = it_2.next()
                key_2 = get_snippet_key(snippet_2)
            elif key_1 < key_2:
                snippet_1 = it_1.next()
                key_1 = get_snippet_key(snippet_1)
            else:
                snippet_2 = it_2.next()
                key_2 = get_snippet_key(snippet_2)
    except StopIteration:
        pass


def save_tasks(taskpool, snip_pairs):
    snip_dict = {}
    task_list = []
    for (snip_1, snip_2) in snip_pairs:
        snip_dict[snip_1.snippet_id] = snip_1
        snip_dict[snip_2.snippet_id] = snip_2
        task_list.append(Task(taskpool=taskpool,
                              first_snippet=snip_1,
                              second_snippet=snip_2,
                              request=snip_1.data['query'],
                              region=snip_1.data['region'],
                              status=Task.Status.REGULAR,
                              shuffle=generate_shuffle(taskpool)))
    if task_list:
        all_keys = set(Snippet.objects.filter(
            Q(snippet_id__startswith=taskpool.first_pool_id.split('_')[0]) |
            Q(snippet_id__startswith=taskpool.second_pool_id.split('_')[0])
        ).values_list('snippet_id', flat=True))
        new_keys = set(snip_dict.keys()) - all_keys
        Snippet.objects.bulk_create([snip_dict[key] for key in new_keys], batch_size=BULK_CREATE_BATCH_SIZE)
        Task.objects.bulk_create(task_list, batch_size=BULK_CREATE_BATCH_SIZE)
    return len(task_list)


def generate_shuffle(taskpool):
    if taskpool.kind != TaskPool.Type.BLIND:
        return False
    return bool(random.randrange(2))


def define_countries(taskpool, countries):
    TPCountry.objects.bulk_create([
        TPCountry(taskpool=taskpool, country=country) for country in countries
    ])


def generate(request, ignore_bold):
    if request.POST['kind_pool'] == TaskPool.TypePool.RCA:
        first_pool_name = 'first_pool_rca'
        second_pool_name = 'second_pool_rca'
    else:
        first_pool_name = 'first_pool'
        second_pool_name = 'second_pool'
    snippet_pool_obj_1 = get_object_or_404(SnippetPool,
                                           pk=request.POST[first_pool_name])
    snippet_pool_obj_2 = get_object_or_404(SnippetPool,
                                           pk=request.POST[second_pool_name])
    snippet_pool_1_id = snippet_pool_obj_1.storage_id
    snippet_pool_2_id = snippet_pool_obj_2.storage_id
    taskpool = get_taskpool_obj(request, snippet_pool_obj_1,
                                snippet_pool_obj_2)
    taskpool.save()
    define_countries(taskpool, request.POST.getlist('country'))
    task_snippets = task_snippets_generator(request, snippet_pool_1_id,
                                            snippet_pool_2_id, ignore_bold)
    taskpool.count = save_tasks(taskpool, task_snippets)
    if taskpool.count:
        taskpool.save()
        return taskpool.count
    else:
        taskpool.delete()
        return 0

import re

import django.template
import django.utils.safestring as dus


register = django.template.Library()

CMS_MUTATION_SEARCH_REGEX = re.compile(r'^"[a-zA-Z]+"\sin\s"[_a-z0-9-]+"')


@register.filter
def reason_to_link(lock):
    url = None

    if lock.holder == 'mdb-cms-wall-e' and lock.reason.isnumeric():
        url = f'/ui/cms/decisions/{lock.reason}'
    elif lock.holder == 'cms' and CMS_MUTATION_SEARCH_REGEX.match(lock.reason):
        url = f'/ui/cms/decisions?mutations_log={lock.lock_ext_id}'
    elif lock.holder == 'dbaas-worker' and lock.reason.startswith('task-'):
        task_id = lock.reason.split('-')[1]
        url = f'/ui/meta/worker_tasks/{task_id}'
    elif lock.holder == 'mdb-cms-instance':
        url = f'/ui/cms/instance_operations/{lock.reason}'

    if url:
        return dus.mark_safe(
            f'<a href="{url}">{lock.reason}</a>'
        )
    else:
        return lock.reason

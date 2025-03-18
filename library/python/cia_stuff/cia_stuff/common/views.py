# coding: utf-8

from __future__ import unicode_literals

import importlib
import json

from django import http
from django.conf import settings
from django import forms
from django.template import Template, RequestContext
from django.views.generic.edit import FormView


TPL_WITH_FORM = """
<form method="post">{% csrf_token %}
    {{ form.as_p }}
    <input type="submit" value="Put task to queue" />
</form>
"""


class CeleryTasksView(FormView):

    def render_to_response(self, context, **response_kwargs):
        tpl = Template(TPL_WITH_FORM)
        rendered = tpl.render(context=RequestContext(self.request, context))
        return http.HttpResponse(rendered)

    def get_form_class(self):
        class CeleryTasksForm(forms.Form):
            task = forms.ChoiceField(choices=sorted([
                (name, name) for name in get_all_celery_tasks()
            ]))
            params_json = forms.CharField(
                widget=forms.Textarea,
                required=False,
            )
        return CeleryTasksForm

    def form_valid(self, form):
        task_name = form.cleaned_data['task']
        try:
            task_kwargs = json.loads(form.cleaned_data['params_json'] or '{}')
        except ValueError as exc:
            return http.JsonResponse({'json parse error': str(exc)})

        task = get_all_celery_tasks()[task_name]
        task.delay(**task_kwargs)
        return http.JsonResponse({
            'delayed task': task_name,
            'kw': task_kwargs
        })

    def form_invalid(self, form):
        return http.JsonResponse(form.errors)


def get_all_celery_tasks():
    root_package = {
        'review': 'review',
        'feedback': 'fb',
        'goals': 'goals',
    }.get(settings.PROJECT_NAME, settings.PROJECT_NAME)

    celery_module = importlib.import_module(root_package + '.celery_app')
    if celery_module is None:
        return {}

    if not hasattr(celery_module, 'app'):
        return {}

    celery_app = celery_module.app
    celery_app.autodiscover_tasks(force=True)
    return celery_app.tasks

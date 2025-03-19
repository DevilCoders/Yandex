from django.conf import settings
import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.deploy.models as mod_models
import cloud.mdb.backstage.apps.deploy.filters as mod_filters


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class JobResultSkelView(django.views.generic.View):
    def get(self, request, job_result_id, section='common'):
        job_result = mod_models.JobResult.objects.get(pk=job_result_id)
        ctx = {
            'menu': 'Deploy',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': job_result,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class JobResultSectionView(django.views.generic.View):
    def get(self, request, job_result_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(job_result_id)
        response.html = mod_helpers.render_html(f'deploy/job_results/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, job_result_id):
        return {
            'changes_read_permission': settings.CONFIG.apps.deploy.get('job_result_changes_read_permission'),
            'obj': mod_models.JobResult.objects.get(pk=job_result_id)
        }


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class JobResultBlockView(django.views.generic.View):
    def get(self, request, job_result_id, block):
        BLOCKS = {
            'job': self.get_job_context,
        }
        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(job_result_id)
        response.html = mod_helpers.render_html(f'deploy/job_results/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_job_context(self, job_result_id):
        job_result = mod_models.JobResult.objects.get(pk=job_result_id)
        job = mod_models.Job.objects.filter(
            ext_job_id=job_result.ext_job_id
        ).select_related(
            'command',
            'command__shipment_command',
            'command__shipment_command__shipment',
        )\
        .first()

        return {'job': job}


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class JobResultListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Deploy',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_job_results_filters(request),
            'onload_url': '/ui/deploy/ajax/job_results',
            'title': 'JobResults',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class JobResultListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_job_results_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'deploy_job_results',
                'array_name': 'deploy_job_results_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.JobResult.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('deploy/job_results/job_results.html', ctx, request)
        return http.JsonResponse(response)

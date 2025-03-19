from django.conf import settings

import django.http as http
import django.views.generic
import django.utils.decorators

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.apps as apps
import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.meta.models as mod_models
import cloud.mdb.backstage.apps.meta.filters as mod_filters
import cloud.mdb.backstage.apps.meta.actions.worker_tasks as mod_actions


if apps.META.is_enabled:
    META_ADMIN_PERM = settings.CONFIG.apps.meta.admin_permission
else:
    META_ADMIN_PERM = None


def has_changes(old_rev, new_rev, pillar_changes):
    if any(pillar_changes):
        return True

    if not old_rev and not new_rev:
        return False

    if type(old_rev) == dict and type(new_rev) == dict:
        for k, v in old_rev.items():
            if k not in {'rev', 'created_at'} and v != new_rev.get(k):
                return True
        return False

    return True


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class TaskSkelView(django.views.generic.View):
    def get(self, request, task_id, section='common'):
        task = mod_models.WorkerTask.objects.get(pk=task_id)
        ctx = {
            'menu': 'Meta',
            'section': section,
            'get_params': request.GET.urlencode(),
            'obj': task,
            'sidebar_collapse': True,
        }
        html = mod_helpers.render_html('lib/object.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class TaskSectionView(django.views.generic.View):
    def get(self, request, task_id, section='common'):
        SECTIONS = {
            'common': self.get_common_context,
        }

        response = mod_response.Response()
        method = SECTIONS[section]
        ctx = method(task_id, request)
        response.html = mod_helpers.render_html(f'meta/worker_tasks/sections/{section}.html', ctx, request)
        return http.JsonResponse(response)

    def get_common_context(self, task_id, request):
        if request.iam_user.has_perm(META_ADMIN_PERM):
            actions = mod_models.WorkerTaskAction.all
        else:
            actions = []
        return {
            'obj': mod_models.WorkerTask.objects.get(pk=task_id),
            'actions': actions,
        }


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class TaskBlockView(django.views.generic.View):
    def get(self, request, task_id, block):
        BLOCKS = {
            'shipments': self.get_shipments_context,
            'acquire_finish_changes': self.get_acquire_finish_changes_context,
            'required_task': self.get_required_task_context,
            'restarts': self.get_restarts_context,
        }

        response = mod_response.Response()
        method = BLOCKS[block]
        ctx = method(task_id)
        response.html = mod_helpers.render_html(f'meta/worker_tasks/blocks/{block}.html', ctx, request)
        return http.JsonResponse(response)

    def get_required_task_context(self, task_id):
        worker_task = mod_models.WorkerTask.objects.get(pk=task_id)
        return {'task': mod_models.WorkerTask.objects.get(pk=worker_task.required_task)}

    def get_shipments_context(self, task_id):
        worker_task = mod_models.WorkerTask.objects.get(pk=task_id)
        return {'jobs': worker_task.get_shipments_jobs()}

    def get_restarts_context(self, task_id):
        restarts = mod_models.WorkerTaskRestartHistory.ui_list.filter(task_id=task_id).order_by('restart_count')
        return {'restarts': restarts}

    def get_acquire_finish_changes_context(self, task_id):
        data = {}
        changes_count = 0
        worker_task = mod_models.WorkerTask.objects.get(pk=task_id)
        data['cluster'] = self._get_cluster_afc(worker_task)
        data['subclusters'] = self._get_subclusters_afc(worker_task)
        data['hosts'] = self._get_hosts_afc(worker_task)
        for _, value in data.items():
            if value:
                changes_count += 1

        return {'data': data, 'changes_count': changes_count}

    def _get_cluster_afc(self, worker_task):
        result = None
        cluster = worker_task.cluster

        pillar_revs = [
            mod_models.PillarRev.get_prev_rev(worker_task.create_rev - 1, cid=cluster.cid),
            mod_models.PillarRev.get_prev_rev(worker_task.finish_rev, cid=cluster.cid),
        ]

        pillar_changes = mod_models.PillarRev.get_changes(pillar_revs, with_revs=False)
        old_rev = cluster.get_prev_rev(worker_task.create_rev -1)
        new_rev = cluster.get_prev_rev(worker_task.finish_rev)

        if has_changes(old_rev, new_rev, pillar_changes):
            result = {
                'cluster': cluster,
                'old_rev': old_rev,
                'new_rev': new_rev,
                'pillar_changes': pillar_changes,
            }

        return result

    def _get_subclusters_afc(self, worker_task):
        result = []
        cluster = worker_task.cluster
        for subcid in cluster.get_historical_subclusters():
            pillar_revs = [
                mod_models.PillarRev.get_prev_rev(worker_task.create_rev - 1, subcid=subcid),
                mod_models.PillarRev.get_prev_rev(worker_task.finish_rev, subcid=subcid),
            ]
            pillar_changes = mod_models.PillarRev.get_changes(pillar_revs, with_revs=False)
            old_rev = mod_models.Subcluster.get_prev_rev(subcid, worker_task.create_rev - 1)
            new_rev = mod_models.Subcluster.get_prev_rev(subcid, worker_task.finish_rev)

            if has_changes(old_rev, new_rev, pillar_changes):
                result.append({
                    'subcluster': mod_models.Subcluster.objects.get(pk=subcid),
                    'old_rev': old_rev,
                    'new_rev': new_rev,
                    'pillar_changes': pillar_changes,
                })
        return result

    def _get_hosts_afc(self, worker_task):
        result = []
        cluster = worker_task.cluster
        for fqdn in cluster.get_historical_hosts():
            pillar_revs = [
                mod_models.PillarRev.get_prev_rev(worker_task.create_rev - 1, fqdn=fqdn),
                mod_models.PillarRev.get_prev_rev(worker_task.finish_rev, fqdn=fqdn),
            ]
            pillar_changes = mod_models.PillarRev.get_changes(pillar_revs, with_revs=False)
            old_rev = mod_models.Host.get_prev_rev(fqdn, worker_task.create_rev - 1)
            new_rev = mod_models.Host.get_prev_rev(fqdn, worker_task.finish_rev)

            if has_changes(old_rev, new_rev, pillar_changes):
                result.append({
                    'fqdn': fqdn,
                    'old_rev': old_rev,
                    'new_rev': new_rev,
                    'pillar_changes': pillar_changes,
                })
        return result


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class TaskListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'Meta',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_worker_tasks_filters(request),
            'onload_url': '/ui/meta/ajax/worker_tasks',
            'title': 'Worker Tasks',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class TaskListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_worker_tasks_filters(request)

        if request.iam_user.has_perm(META_ADMIN_PERM):
            actions = mod_models.WorkerTaskAction.all
        else:
            actions = []

        ctx = {
            'filters': filters,
            'model': mod_models.WorkerTask,
            'actions': actions,
            'js': {
                'identifier': 'meta_worker_tasks',
                'array_name': 'meta_worker_tasks_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            ctx = mod_helpers.paginator_setup_set_context(
                queryset=mod_models.WorkerTask.ui_list.filter(**filters.queryset),
                request=request,
                context=ctx,
            )

        response.html = mod_helpers.render_html('meta/worker_tasks/worker_tasks.html', ctx, request)
        return http.JsonResponse(response)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
@django.utils.decorators.method_decorator(iam_decorators.permission_required(META_ADMIN_PERM), name='dispatch')
class TaskDialogView(django.views.generic.View):
    def post(self, request, action):
        response = mod_response.Response()

        if action not in mod_models.WorkerTaskAction.map.keys():
            response.mark_failed(f'Unknown action: {action}')

        else:
            post_data = mod_helpers.get_post_data(request)

            objects = mod_models.WorkerTask.ui_list.filter(pk__in=post_data['ids'])

            ctx = {
                'action': mod_models.WorkerTaskAction.map[action],
                'action_ability': all([o.action_ability(action)[0] for o in objects]),
                'objects': objects,
                'js': {
                    'array_name': post_data['array_name'],
                    'app': post_data['app'],
                    'model': post_data['model'],
                },
            }

            response.html = mod_helpers.render_html('meta/worker_tasks/action_dialog.html', ctx, request)
        return http.JsonResponse(response)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
@django.utils.decorators.method_decorator(iam_decorators.permission_required(META_ADMIN_PERM), name='dispatch')
class TaskActionView(django.views.generic.View):
    def post(self, request, action):
        response = mod_response.Response()

        if action not in mod_models.WorkerTaskAction.map.keys():
            response.mark_failed(f'Unknown action: {action}')
        else:
            post_data = mod_helpers.get_post_data(request)

            results = mod_actions.process_action(
                objects=mod_models.WorkerTask.ui_list.filter(pk__in=post_data['ids']),
                action=action,
                action_params=post_data['inputs'],
                username=request.iam_user.login,
                client_ip=mod_helpers.get_client_ip(request),
                request_id=mod_helpers.get_request_id(request),
            )

            for result in results:
                if not result:
                    response.mark_failed(result.obj_error_html, safe=True)
                else:
                    response.add_msg(result.obj_msg_html, safe=True)

        return http.JsonResponse(response)

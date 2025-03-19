import django.db
import django.http as http

import cloud.mdb.backstage.apps.iam.decorators as iam_decorators

import cloud.mdb.backstage.lib.helpers as mod_helpers
import cloud.mdb.backstage.lib.response as mod_response

import cloud.mdb.backstage.apps.mlock.filters as mod_filters
import cloud.mdb.backstage.apps.mlock.lock as mod_lock


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class LockListSkelView(django.views.generic.View):
    def get(self, request):
        ctx = {
            'menu': 'MLock',
            'get_params': request.GET.urlencode(),
            'filters': mod_filters.get_locks_filters(request),
            'onload_url': '/ui/mlock/ajax/locks',
            'title': 'Locks',
        }
        html = mod_helpers.render_html('lib/objects_list.html', ctx, request)
        return http.HttpResponse(html)


@django.utils.decorators.method_decorator(iam_decorators.auth_required, name='dispatch')
class LockListView(django.views.generic.View):
    def get(self, request):
        response = mod_response.Response()
        filters = mod_filters.get_locks_filters(request)

        ctx = {
            'filters': filters,
            'js': {
                'identifier': 'mlock_locks',
                'array_name': 'mlock_locks_selected',
                'callback_func': 'activate_action_buttons_panel',
            }
        }

        if not filters.errors:
            locks = mod_lock.Lock.get_locks()
            fqdn_regex = filters.regex.get('fqdn')
            if fqdn_regex:
                _locks = []
                for lock in locks:
                    for fqdn in lock.objects:
                        if fqdn_regex.search(fqdn):
                            _locks.append(lock)
                            break
                locks = _locks

            for name in ['lock_ext_id', 'holder', 'reason']:
                regex = filters.regex.get(name)
                if regex:
                    locks = filter(lambda l: regex.search(getattr(l, name)), locks)
                    locks = list(locks)

            ctx = mod_helpers.paginator_setup_set_context(
                queryset=locks,
                request=request,
                context=ctx,
                items_per_page=200,
            )

        response.html = mod_helpers.render_html('mlock/locks.html', ctx, request)
        return http.JsonResponse(response)

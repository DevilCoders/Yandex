import json
import math

from django import forms
from django.conf import settings
from django.templatetags.static import static
from django.utils.safestring import mark_safe
from nested_inline.admin import NestedModelAdmin, NestedStackedInline, NestedTabularInline

from cloud.mdb.ui.internal.mdbui.installation import InstallationType


class LinkedMixin:
    @property
    def link(self):
        return with_copy(
            str(self),
            '<a href="/{app}/{model}/{id}">{name}</a>'.format(
                app=self.__class__._meta.app_label, model=self.__class__._meta.model_name, id=self.pk, name=str(self)
            ),
        )

    @property
    def link_ext(self):
        _meta = self.__class__._meta
        return mark_safe(
            '<a href="/{app}/{model}/{id}">{name}</a>'.format(
                app=_meta.app_label,
                model=_meta.model_name,
                id=self.pk,
                name='%s -> %s -> %s' % (_meta.app_label.title(), _meta.verbose_name_plural.title(), str(self)),
            )
        )


def with_copy(data, link=None):
    return mark_safe(
        """{link}
        <button class="btn-copy" data-clipboard-action="copy" data-clipboard-text="{data}" type='reset'>&#10064;</button>
    """.format(
            data=str(data),
            link=link if link else data,
        )
    )


class ROAdmin(NestedModelAdmin):
    def get_readonly_fields(self, request, obj=None):
        return self.fields or []

    def has_delete_permission(self, request, obj=None):
        return False

    def has_add_permission(self, request):
        return False

    def render_change_form(self, request, context, add=False, change=False, form_url='', obj=None):
        if obj:
            context['title'] = context['title'][7:].capitalize() + ' ' + str(obj)
        form = super().render_change_form(request, context, add, change, form_url, obj)
        return form

    def change_view(self, request, object_id, form_url='', extra_context=None):
        if not extra_context:
            extra_context = {'has_permission': True}
        else:
            extra_context.update({'has_permission': True})
        return super().change_view(request, object_id, form_url, extra_context)


class ROTablularInline(NestedTabularInline):
    extra = 0
    can_delete = False

    def has_add_permission(self, request, obj=None):
        return False

    def get_readonly_fields(self, request, obj=None):
        return self.fields

    @property
    def media(self):
        # otherwise self.classes doesn't work
        extra = '' if settings.DEBUG else '.min'
        js = ['vendor/jquery/jquery%s.js' % extra, 'jquery.init.js', 'inlines-nested%s.js' % extra]
        if self.prepopulated_fields:
            js.extend(['urlify.js', 'prepopulate%s.js' % extra])
        if self.filter_vertical or self.filter_horizontal:
            js.extend(['SelectBox.js', 'SelectFilter2.js'])
        if self.classes and 'collapse' in self.classes:
            js.append('collapse%s.js' % extra)
        return forms.Media(js=[static('admin/js/%s' % url) for url in js])


class ROStackedInline(NestedStackedInline):
    extra = 0
    can_delete = False

    def has_add_permission(self, request, obj=None):
        return False

    def get_readonly_fields(self, request, obj=None):
        return self.fields


def _change_to_json(change):
    if type(change) == list:
        try:
            if type(change[0]) != int and len(change) == 2:
                return json.dumps({change[0]: change[1]})
            elif all([len(row) == 2 for row in change]):
                return json.dumps({row[0]: row[1] for row in change})
        except:
            pass
    return json.dumps(change)


def render_change(change):
    result = []
    for op, pos, ch in change:
        if op == 'add':
            result.append(
                "<span class='plus'>+++ %s</span><pre class='json json-mini'>%s</pre>" % (pos, _change_to_json(ch))
            )
        elif op == 'remove':
            result.append(
                "<span class='minus'>--- %s</span><pre class='json json-mini'>%s</pre>" % (pos, _change_to_json(ch))
            )
        elif op == 'change':
            # check if some changes
            result.append(
                "<span class='minus'>--- %s</span><pre class='json json-mini'>%s</pre>" % (pos, _change_to_json(ch[0]))
            )
            result.append(
                "<span class='plus'>+++ %s</span><pre class='json json-mini'>%s</pre>" % (pos, _change_to_json(ch[1]))
            )
    return '<br />'.join(result)


def make_spoiler(title, data):
    return """<details>
        <summary>{title}</summary>
        {data}
    </details>""".format(
        title=title, data=data
    )


def render_pillar_revs(changes):
    if all([not change for _, change in changes]):
        return

    data = (
        """<table>
            <thead><tr>
                <th>Previous&nbsp;Rev</th>
                <th>Rev</th>
                <th>Change</th>
            </tr>
            <tbody>"""
        + ''.join(
            [
                "<tr><td>{prev_rev}</td><td>{rev}</td><td>{change}</td></tr>".format(
                    prev_rev=rev - 1,
                    rev=rev,
                    change=render_change(change),
                )
                for rev, change in reversed(changes)
                if change
            ]
        )
        + "</tbody></table>"
    )

    if len(data) > 4096:
        return make_spoiler('Pillar revisions here', data)
    else:
        return data


def render_pillar(pillar_value):
    data = '<pre class="json">' + json.dumps(pillar_value) + '</pre>'
    if len(data) > 4096:
        return make_spoiler('Pillar here', data)
    else:
        return data


def make_jaeger_link(tracing, installation):
    trace_id = tracing.split(':')[1].strip()[1:]
    if installation == InstallationType.porto_test:
        return '<a href="https://jaeger.yt.yandex-team.ru/mdb-testing/trace/%s">%s</a>' % (trace_id, trace_id)
    elif installation == InstallationType.porto_prod:
        return '<a href="https://jaeger.yt.yandex-team.ru/mdb-prod/trace/%s">%s</a>' % (trace_id, trace_id)
    elif installation == InstallationType.compute_prod:
        return '<a href="https://jaeger.private-api.ycp.cloud.yandex.net/trace/%s">%s</a>' % (trace_id, trace_id)
    elif installation == InstallationType.compute_preprod:
        return '<a href="https://jaeger-ydb.private-api.ycp.cloud-preprod.yandex.net/trace/%s">%s</a>' % (
            trace_id,
            trace_id,
        )


def render_job_result(data):
    result = """
    <table class="table">
    <thead>
      <tr>
        <th>id</th>
        <th>changes</th>
      </tr>
    </thead>
    <tbody>"""
    for row in data.get('return'):
        if type(row) != dict:
            result += row
            continue
        if not row['result']:
            result += '<tr class="danger">'
        elif row['result'] and (row['changes'] == '{}' or not row['changes']):
            result += '<tr class="success">'
        else:
            result += '<tr class="info">'

        state_id = row.get('__id__')
        if 'name' in row and 'state' in row:
            state = row['state'].split('_|-')
            if len(state) > 2:
                state_id = '%s.%s %s' % (state[0], state[-1], row.get('name'))
        result += '<th>%s</th><td>' % state_id

        if 'diff' in row['changes']:
            result += '<pre><code class="diff">%s</code></pre>' % row['changes']['diff']
        else:
            for k, v in row['changes'].items():
                if type(v) == dict and 'diff' in v:
                    result += '%s:<br>' % k
                    result += '<pre><code class="diff">%s</code></pre>' % v['diff']
                else:
                    result += '%s: %s<br>' % (k, v)
        if row['comment']:
            if type(row['comment']) == list:
                result += 'Comment: <pre><code>%s</code></pre>' % '\n'.join(row['comment'])
            else:
                result += 'Comment: <pre><code>%s</code></pre>' % row['comment']
        result += '</td></tr>' % row

    result += "</tbody></table>"
    return result


def get_cluster_links(cluster_type, cid, installation: InstallationType):
    result = []

    cluster_type_suffix = cluster_type[: -len('_cluster')]
    if cluster_type == 'postgresql_cluster':
        cluster_type_suffix_short = 'postgres'
    else:
        cluster_type_suffix_short = cluster_type_suffix

    if installation.is_porto():
        yc_base = 'yc' if installation == InstallationType.porto_prod else 'yc-test'
        result.extend(
            [
                (
                    'YC UI',
                    'https://{yc_base}.yandex-team.ru/folders/mdb-junk/managed-{prefix}/cluster/{cid}'.format(
                        cid=cid, yc_base=yc_base, prefix=cluster_type_suffix
                    ),
                ),
                (
                    'Solomon',
                    'https://solomon.yandex-team.ru/?cluster=mdb_{cid}&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-{prefix}'.format(
                        cid=cid, prefix=cluster_type_suffix_short
                    ),
                ),
                (
                    'YASM',
                    'https://yasm.yandex-team.ru/template/panel/dbaas_{prefix}_metrics/cid={cid}'.format(
                        cid=cid, prefix=cluster_type_suffix_short
                    ),
                ),
            ]
        )

    elif installation == InstallationType.compute_prod:
        result.extend(
            [
                (
                    'YC UI',
                    'https://console.cloud.yandex.ru/folders/b1g0r9fh49hee3rsc0aa/managed-{prefix}/cluster/{cid}'.format(
                        cid=cid, prefix=cluster_type_suffix
                    ),
                ),
                (
                    'Solomon',
                    'https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=mdb_{cid}&service=yandexcloud_dbaas&dashboard=cloud-mdb-cluster-{prefix}'.format(
                        cid=cid, prefix=cluster_type_suffix_short
                    ),
                ),
            ]
        )
    elif installation == InstallationType.compute_preprod:
        result.extend(
            [
                (
                    'YC UI',
                    'https://console-preprod.cloud.yandex.ru/folders/aoe3pfp0comvds8269mr/managed-{prefix}/cluster/{cid}'.format(
                        cid=cid, prefix=cluster_type_suffix
                    ),
                ),
            ]
        )
    return result


def get_host_links(host, cid, installation: InstallationType):
    if installation.is_porto():
        return [
            (
                'Solomon DOM0',
                'https://solomon.yandex-team.ru/?cluster=internal-mdb_dom0&dc=by_cid_container&host=by_cid_container'
                '&project=internal-mdb&service=dom0&dashboard=internal-mdb-porto-instance&l.container='
                '{host}&b=1h&e='.format(host=host),
            )
        ]
    elif installation == InstallationType.compute_prod:
        return [
            (
                'DOM0',
                'https://solomon.cloud.yandex-team.ru/?project=yandexcloud&service=yandexcloud_dbaas&dashboard=cloud'
                '-mdb-instance-system&cluster=mdb_{cid}&b=1d&e=&l.host={host}'.format(cid=cid, host=host),
            )
        ]
    return []


def render_table(titles, data, full_width=False):
    if not data:
        return '-'
    thead = '\n'.join(['<th>%s</th>' % title for title in titles])
    tbody = []
    for row in data:
        tbody.append(''.join(['<td>%s</td>' % x for x in row]))
    tbody = '\n'.join(['<tr>%s</tr>' % row for row in tbody])

    return """<table %s>
        <thead>
            %s
        </thead>
        <tbody>
            %s
        </tbody>
    </table>""" % (
        'style="width: 100%"' if full_width else '',
        thead,
        tbody,
    )


def size_pretty(size_bytes):
    if size_bytes is None:
        return '-'
    if size_bytes == 0:
        return "0"
    size_name = ("B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB")
    i = int(math.floor(math.log(size_bytes, 1024)))
    p = math.pow(1024, i)
    s = round(size_bytes / p, 2)
    return "%s %s" % (s, size_name[i])

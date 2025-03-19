import six
import uuid
import json
import base64

import sqlparse
import pygments
import pygments.lexers
import pygments.formatters

from django.conf import settings
import django.template
import django.utils.timezone as timezone
import django.utils.dateparse as dateparse
import django.utils.safestring as dus

import metrika.pylib.utils as mtu

import cloud.mdb.backstage.lib.helpers as mod_helpers


register = django.template.Library()


@register.filter
def to_managed_fqdn(fqdn):
    if settings.INSTALLATION.managed_domain == settings.INSTALLATION.unmanaged_domain:
        return fqdn

    if not settings.INSTALLATION.managed_domain or not settings.INSTALLATION.unmanaged_domain:
        return fqdn

    return fqdn.replace(settings.INSTALLATION.unmanaged_domain, settings.INSTALLATION.managed_domain)


@register.simple_tag
def action_ability(action, obj):
    value = ''
    try:
        result, err = obj.action_ability(action)
    except Exception as err:
        value = f'failed to check action ability: {err}'
    else:
        if not result:
            value = err

    if value:
        return dus.mark_safe(
            f'<span class="red">{value}</span>'
        )
    else:
        return dus.mark_safe('&ndash;')


@register.filter
def action_ability_check(obj, action):
    try:
        result, _ = obj.action_ability(action)
    except Exception:
        return False
    else:
        return result


@register.filter
def action_ability_check_message(obj, action):
    try:
        _, message = obj.action_ability(action)
    except Exception as err:
        return str(err)
    else:
        return message


@register.simple_tag
def obj_link(obj, name):
    return dus.mark_safe(
        f'<a href="{obj.self_url}">{name}</a>'
    )


@register.simple_tag
def tab_item(key, active=False, name=None):
    if name is None:
        name = key.lower().replace('_', ' ').title()

    if active:
        active_cls = ' class="active"'
    else:
        active_cls = ''

    return dus.mark_safe(
        f'<li{active_cls}><a href="#tab_{key}" data-toggle="tab">{name} <span id="tab_{key}_counter"></span></a></li>'
    )


@register.simple_tag
def action_input(name, action, required=False):
    ctx = {
        'name': name,
        'action': action,
        'required': required,
    }
    template = django.template.loader.get_template(
        'lib/templatetags/action_input.html'
    )
    html = template.render(ctx)
    return dus.mark_safe(html)


@register.simple_tag
def collapsible(tpl, text="show", close_text="hide", **kwargs):
    tag_template = django.template.loader.get_template(tpl)
    data = dus.mark_safe(tag_template.render(kwargs))
    ctx = {
        'text': text,
        'close_text': close_text,
        'data': data,
        'unique_value': str(uuid.uuid4()),
    }
    template = django.template.loader.get_template(
        'lib/templatetags/collapsible.html'
    )
    html = template.render(ctx)
    return dus.mark_safe(html)


@register.simple_tag
def update_tab_badge(key, value):
    return dus.mark_safe(f"<script>update_badge_count('{key}', '{value}')</script>")


@register.simple_tag
def selectable_th_checkbox(
    identifier,
    array_name,
    callback_func,
):
    ctx = {
        'identifier': identifier,
        'array_name': array_name,
        'callback_func': callback_func,
    }
    template = django.template.loader.get_template(
        'lib/templatetags/selectable_th_checkbox.html'
    )
    html = template.render(ctx)
    return dus.mark_safe(html)


@register.simple_tag
def selectable_td_checkbox(
    key,
    identifier,
    array_name,
    callback_func,
):
    ctx = {
        'key': key,
        'identifier': identifier,
        'array_name': array_name,
        'callback_func': callback_func,
    }
    template = django.template.loader.get_template(
        'lib/templatetags/selectable_td_checkbox.html'
    )
    html = template.render(ctx)
    return dus.mark_safe(html)


@register.filter
def default_label(value):
    return dus.mark_safe(f'<span class="label backstage-label backstage-label-default">{value}</span>')


@register.filter()
def dict_key(data, key):
    return data[key]


@register.filter()
def get(data, key):
    return data.get(key)


@register.simple_tag
def show_sql(value):
    value = sqlparse.format(str(value), reindent=True, keyword_case='upper')
    value = base64.b64encode(value.encode()).decode()
    html = f'<i class="fas fa-info-circle fa-fw noodle-js-copy" onClick="show_sql(\'{value}\')"></i>'
    return dus.mark_safe(html)


@register.simple_tag
def simple_copy(value):
    html = mod_helpers.get_simple_copy_html(value)
    return dus.mark_safe(html)


@register.simple_tag
def simple_info_tooltip(value):
    html = f'<i class="fas fa-info-circle fa-fw noodle-info-sign" data-toggle="tooltip" data-placement="right" title="{value}"></i>'
    return dus.mark_safe(html)


@register.simple_tag
def age_span(dt, seconds=None):
    if dt is None:
        return ''
    color = 'green'
    now = timezone.now()
    delta = now - dt
    if seconds:
        if delta.total_seconds() > seconds:
            color = 'red'
    str_delta = mtu.timedelta_to_str(delta)
    return dus.mark_safe(
        '<span class="{}" style="font-size: 10px;">{}</span>'.format(color, str_delta)
    )


@register.filter
def yauser_link_format(user):
    if not user:
        return user
    if not isinstance(user, six.string_types):
        username = user.get_username()
    else:
        username = user
    first_letter = username[0]
    rest_letters = username[1:]
    link = (
        f'<a class="noodle-yauser-link" target="_blank" href="https://staff.yandex-team.ru/{username}" '
        f'data-staff="{username}"><span class="noodle-yauser-link-first-letter">{first_letter}</span>{rest_letters}</a>'
    )
    return dus.mark_safe(link)


@register.filter
def bool_label(value):
    if value is None:
        cls = 'warning'
        value = 'None'
    elif value:
        cls = 'success'
        value = 'True'
    else:
        cls = 'error'
        value = 'False'
    return dus.mark_safe(f'<span class="label backstage-label backstage-label-{cls}">{value}</span>')


@register.simple_tag
def dt_formatted(dt, inline=False):
    if isinstance(dt, str):
        try:
            dt = dateparse.parse_datetime(dt)
            if timezone.is_naive(dt):
                dt = timezone.make_aware(dt, timezone.utc)
        except Exception:
            return dt
    ctx = {
        'dt': dt,
        'inline': inline,
    }
    template = django.template.loader.get_template(
        'lib/templatetags/dt_formatted.html'
    )
    html = template.render(ctx)
    return dus.mark_safe(html)


@register.simple_tag
def pillar_change(change, change_key_1, change_key_2=None):
    ctx = {
        'change': change,
        'change_id': f'{change_key_1}{change_key_2}',
    }
    template = django.template.loader.get_template(
        'lib/templatetags/pillar_change.html'
    )
    html = template.render(ctx)
    return dus.mark_safe(html)


@register.filter
def obj_app_label(obj):
    return obj.__class__._meta.app_label


@register.filter
def obj_model_label(obj):
    return obj.__class__._meta.verbose_name_plural


@register.simple_tag
def duration_span(d1, d2):
    if d2 < d1 or d2 == d1:
        str_delta = 0
    else:
        delta = d2 - d1
        str_delta = mtu.timedelta_to_str(delta)
    return dus.mark_safe(
        f'<span class="green" style="font-size: 10px;">{str_delta}</span>'
    )


@register.simple_tag
def obj_block_onload(obj, block):
    return dus.mark_safe(
        f'<span data-onload="True" data-onload-url="{obj.self_ajax_url}/blocks/{block}" data-small-container-loader="True"></span>'
    )


@register.simple_tag
def related_links_onload(fqdn):
    return dus.mark_safe(
        f'<span data-onload="True" data-onload-url="/ui/main/ajax/related_links/{fqdn}" data-small-container-loader="True"></span>'
    )


@register.filter
def to_json(value):
    return json.dumps(value, sort_keys=True, indent=4, default=str)


@register.filter
def dash_if_not(value):
    if not value:
        if value != 0:
            return dus.mark_safe('<span class="gray">&ndash;</span>')
        else:
            return 0
    else:
        return value


@register.filter
def change_to_json(change):
    value = change
    if isinstance(change, list):
        try:
            if not isinstance(change[0], int) and len(change) == 2:
                value = {change[0]: change[1]}
            elif all([len(row) == 2 for row in change]):
                value = {row[0]: row[1] for row in change}
        except:
            pass
    try:
        return json.dumps(value, sort_keys=True, indent=4, default=str)
    except Exception:
        return value


@register.filter
def format_change_pos(pos):
    try:
        if isinstance(pos, six.string_types):
            return pos
        else:
            return ' . '.join([str(x) for x in pos])
    except Exception as err:
        return f'{pos} err: {err}'


@register.filter
def jaeger_link(tracing):
    if not tracing:
        value = '<span class="gray">&ndash;</span>'
    else:
        trace_id = tracing.split(':')[1].strip()[1:]
        path = f'trace/{trace_id}'
        url = None

        if settings.INSTALLATION.is_porto():
            if settings.INSTALLATION.name == 'test':
                url = f'https://jaeger.yt.yandex-team.ru/mdb-testing/{path}'
            elif settings.INSTALLATION.name == 'prod':
                url = f'https://jaeger.yt.yandex-team.ru/mdb-prod/{path}'
        elif settings.INSTALLATION.is_compute():
            if settings.INSTALLATION.name == 'prod':
                url = f'https://jaeger.private-api.ycp.cloud.yandex.net/{path}'
            elif settings.INSTALLATION.name == 'preprod':
                url = f'https://jaeger-ydb.private-api.ycp.cloud-preprod.yandex.net/{path}'
        elif settings.INSTALLATION.is_cloudil():
            if settings.INSTALLATION.name == 'prod':
                url = f'https://jaeger.private-api.yandexcloud.co.il/{path}'

        if url is None:
            return 'No jaeger support yet'

        value = f'<a href="{url}" target="_blank">{trace_id}</a>'
    return dus.mark_safe(value)


@register.filter
def to_html_diff(diff_text):
    return dus.mark_safe(
        pygments.highlight(
            diff_text.strip(),
            pygments.lexers.DiffLexer(),
            pygments.formatters.HtmlFormatter(),
        )
    )


@register.filter
def pygments_json(data):
    try:
        return dus.mark_safe(
            pygments.highlight(
                data,
                pygments.lexers.JsonLexer(),
                pygments.formatters.HtmlFormatter(),
            )
        )
    except Exception:
        return data


@register.filter
def to_html_json(json_data, dash_if_not=False):
    if dash_if_not:
        if not json_data:
            return dus.mark_safe('&ndash;')
    data = json.dumps(
        json_data,
        sort_keys=True,
        indent=4,
        ensure_ascii=False,
        default=str,
    )
    return dus.mark_safe(
        pygments.highlight(
            data,
            pygments.lexers.JsonLexer(),
            pygments.formatters.HtmlFormatter(),
        )
    )


@register.filter
def to_html_traceback(traceback_text):
    if isinstance(traceback_text, list):
        traceback_text = ''.join(traceback_text)
    return dus.mark_safe(
        pygments.highlight(
            traceback_text.strip(),
            pygments.lexers.PythonTracebackLexer(),
            pygments.formatters.HtmlFormatter(),
        )
    )


@register.filter
def pretty_json(json_data):
    return json.dumps(
        json_data,
        sort_keys=True,
        indent=4,
        ensure_ascii=False,
        default=str,
    )


@register.filter
def to_alnum(value):
    return ''.join([c for c in value if c.isalnum()])


@register.filter
def pretty_fsf(value):
    if not value:
        return value
    try:
        number, prefix = value.split()
        number = float(number)
        if number.is_integer():
            number = int(number)
            if number == 0:
                prefix = ''
        return dus.mark_safe(
            f'{number} <span class="gray">{prefix}</span>'
        )
    except Exception:
        return value


@register.filter
def call(obj, name):
    return getattr(obj, name)()

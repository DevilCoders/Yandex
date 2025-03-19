import django.template
import django.utils.safestring as dus

import cloud.mdb.backstage.apps.meta.models as mod_models

register = django.template.Library()


SUPPORTED_IMAGES = [
    'clickhouse',
    'elasticsearch',
    'greenplum',
    'hadoop',
    'kafka',
    'mongodb',
    'mysql',
    'postgresql',
    'redis',
    'sqlserver',
]


@register.filter
def pretty_cluster_type(cluster_type):
    value = mod_models.ClusterType.map.get(cluster_type)
    if value:
        value = value[1]
    else:
        value = cluster_type

    return dus.mark_safe(
        f'<span title="{cluster_type}">{value}</span>'
    )


@register.filter
def clear_roles(roles):
    try:
        return roles.replace('{', '').replace('}', '')
    except Exception:
        return roles


@register.simple_tag
def cluster_ui_status(cluster):
    status = cluster.get_status_display()
    status_cls = cluster.status.replace(' ', '_').lower()
    return dus.mark_safe(
        f'<span class="label backstage-label backstage-meta-cluster-status-{status_cls}">{status}</span>'
    )


@register.simple_tag(takes_context=True)
def cluster_image(context, cluster_type, only_src=False):
    result = ''
    value = mod_models.ClusterType.map.get(cluster_type)
    static_address = context.get('static_address')
    if value and static_address:
        value = value[1].lower().replace(' ', '')
        if value in SUPPORTED_IMAGES:
            src = f'{static_address}/images/{value}.png'
            if not only_src:
                result = dus.mark_safe(
                    f'<img class="cluster_logo" title="{value}" src="{src}"></img>'
                )
            else:
                return src
    return result

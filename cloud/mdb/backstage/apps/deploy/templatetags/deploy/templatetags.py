import django.template
import django.utils.safestring as dus

import cloud.mdb.backstage.apps.deploy.models as mod_models


register = django.template.Library()


@register.filter
def sls_url(sls):
    if not sls:
        return ''

    try:
        sls_file = '/'.join(sls.split('.')) + '.sls'
        url = f'https://a.yandex-team.ru/arc_vcs/cloud/mdb/salt/salt/{sls_file}'
        return dus.mark_safe(
            f'<a class="backstage-sls-url" href="{url}" target="_blank">{sls}</a>'
        )
    except Exception:
        return sls


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
def salt_state_duration_to_seconds(value):
    if not value:
        return 0
    seconds = value / 1000
    if seconds < 0.009:
        item = '< 1/100'
    elif seconds > 10:
        item = round(seconds, 1)
    else:
        item = round(seconds, 2)

    return dus.mark_safe(
        f'<span title="{value}" style="font-size:11px">{item}s</span>'
    )


@register.filter
def job_result_ui_status(job_result):
    if not job_result.status:
        return dus.mark_safe('&ndash;')
    status = job_result.get_status_display()
    cls = job_result.status.replace(' ', '_').lower()
    return dus.mark_safe(
        f'<span class="label backstage-label backstage-deploy-job-result-status-{cls}">{status}</span>'
    )


@register.filter
def command_ui_status(command):
    if not command.status:
        return dus.mark_safe('&ndash;')
    status = command.get_status_display()
    cls = status.lower()
    return dus.mark_safe(
        f'<span class="label backstage-label backstage-deploy-command-{cls}">{status}</span>'
    )


@register.filter
def shipment_ui_status(shipment):
    if not shipment.status:
        return dus.mark_safe('&ndash;')
    status = shipment.get_status_display()
    cls = status.replace(' ', '_').lower()
    return dus.mark_safe(
        f'<span class="label backstage-label backstage-deploy-shipment-{cls}">{status}</span>'
    )


@register.filter
def job_ui_status(job):
    if not job.status:
        return dus.mark_safe('&ndash;')
    status = job.get_status_display()
    cls = status.replace(' ', '_').lower()
    return dus.mark_safe(
        f'<span class="label backstage-label backstage-deploy-job-{cls}">{status}</span>'
    )


@register.filter
def wrap_if_newlines(value):
    if isinstance(value, str):
        if '\n' in value:
            return dus.mark_safe(f'<span style="white-space: pre-wrap;">{value}</span>')
    return value

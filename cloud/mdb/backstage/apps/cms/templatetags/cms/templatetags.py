from django.conf import settings

import re
import django.template
import django.utils.safestring as dus


ST_QUEUES = {
    'NOCRFCS': re.compile('.*(?P<ticket>NOCRFCS-[1-9][0-9]{1,10}).*'),
    'MDB': re.compile('.*(?P<ticket>MDB-[1-9][0-9]{1,10}).*'),
    'MDBSUPPORT': re.compile('.*(?P<ticket>MDBSUPPORT-[1-9][0-9]{1,10}).*'),
    'CLOUDOPS': re.compile('.*(?P<ticket>CLOUDOPS-[1-9][0-9]{1,10}).*'),
    'CLOUD': re.compile('.*(?P<ticket>CLOUD-[1-9][0-9]{1,10}).*'),
}


class LogRecord:
    def __init__(self, line):
        spl = line.split()
        number = spl[0]
        text = ' '.join(spl[1:])
        if not number.isnumeric():
            text = line
            number = ''
            head = ''
        else:
            spl = text.split(':')
            head = spl[0]
            text = ':'.join(spl[1:])

        self.head = head
        self.number = number
        self.text = text


class UnkRecord:
    def __init__(self, line):
        self.number = '-1'
        self.head = ''
        self.text = line


register = django.template.Library()


@register.filter
def remove_at(value):
    if value:
        return str(value).replace('@', '')


@register.simple_tag
def decision_ui_status(decision):
    status = decision.get_status_display()
    status_cls = decision.status.replace(' ', '_').lower()
    return dus.mark_safe(
        f'<span class="label backstage-label backstage-cms-decision-status-{status_cls}">{status}</span>'
    )


@register.simple_tag
def decision_fqdns(decision):
    ctx = {
        'obj': decision
    }
    template = django.template.loader.get_template(
        'cms/templatetags/decision_fqdns.html'
    )
    html = template.render(ctx)
    return dus.mark_safe(html)


@register.simple_tag
def decision_action(decision):
    ctx = {
        'obj': decision
    }
    template = django.template.loader.get_template(
        'cms/templatetags/decision_action.html'
    )
    html = template.render(ctx)
    return dus.mark_safe(html)


@register.simple_tag
def decision_author(decision):
    ctx = {
        'obj': decision
    }
    template = django.template.loader.get_template(
        'cms/templatetags/decision_author.html'
    )
    html = template.render(ctx)
    return dus.mark_safe(html)


@register.simple_tag
def instance_operation_ui_status(instance_operation):
    status = instance_operation.get_status_display()
    status_cls = instance_operation.status.replace(' ', '_').lower()
    return dus.mark_safe(
        f'<span class="label backstage-label backstage-cms-instance-operation-status-{status_cls}">{status}</span>'
    )


@register.simple_tag
def instance_operation_ui_type(instance_operation):
    _type = instance_operation.get_operation_type_display()
    type_cls = instance_operation.operation_type.replace(' ', '_').lower()
    return dus.mark_safe(
        f'<span class="label backstage-label backstage-cms-instance-operation-type-{type_cls}">{_type}</span>'
    )


@register.filter
def instance_id_link(io):
    try:
        if settings.INSTALLATION.is_porto():
            dom0 = io.operation_state['dom0_fqdn']
            host = io.operation_state['fqdn']
            return dus.mark_safe(
                f'dom0: <a href="/ui/dbm/dom0_hosts/{dom0}">{dom0}</a><br>host: <a href="/ui/meta/hosts/{host}">{host}</a>'
            )
        elif settings.INSTALLATION.is_compute():
            host = io.operation_state['fqdn']
            return dus.mark_safe(
                f'instance: {io.instance_id}<br>host: <a href="/ui/meta/hosts/{host}">{host}</a>'
            )
    except Exception:
        pass

    return io.instance_id


@register.filter
def task_or_shipment_link(io):
    try:
        if io.operation_type == 'whip_primary_away':
            shipment_id = io.operation_state['whip_master_step']['shipments']
            if shipment_id:
                url = f'<a href="/ui/deploy/shipments/{shipment_id}">{shipment_id}</a>'
            else:
                url = '&ndash;'
        elif io.operation_type == 'move':
            task_id = io.operation_state['move_instance_step']['task_id']
            if task_id:
                url = f'<a href="/ui/meta/worker_tasks/{task_id}">{task_id}</a>'
            else:
                url = '&ndash;'
        else:
            url = 'unsupported operation type'
        return dus.mark_safe(url)
    except Exception:
        return 'err'


@register.simple_tag
def pretty_log(value):
    if not value:
        return dus.mark_safe("&ndash;")

    records = []
    for line in value.splitlines():
        try:
            records.append(LogRecord(line))
        except Exception as err:
            line = f'{line} (failed to parse: {err})'
            records.append(UnkRecord(line))

    ctx = {
        'records': records
    }
    template = django.template.loader.get_template(
        'cms/templatetags/pretty_log.html'
    )
    html = template.render(ctx)
    return dus.mark_safe(html)


@register.filter
def st_link(value):
    for name in ST_QUEUES.keys():
        if name in value:
            match = ST_QUEUES[name].match(value)
            if match:
                ticket = match.group('ticket')
                ticket_link = f'<a href="https://st.yandex-team.ru/{ticket}" target="_blank">{ticket}</a>'
                value = value.replace(ticket, ticket_link)
                value = dus.mark_safe(value)
    return value

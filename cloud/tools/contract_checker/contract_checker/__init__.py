# coding: utf-8

from __future__ import unicode_literals

import argparse
import collections
import logging
import time
import yaml
from library.python import resource
from google.protobuf.json_format import ParseDict

from search.martylib.business_hours import BusinessHours
from search.martylib.core.date_utils import get_datetime, now
from search.martylib.core.exceptions import format_exception
from search.martylib.core.logging_utils import configure_basic_logging
from search.martylib.staff import StaffClient
from startrek_client import Startrek

from cloud.tools.contract_checker.contract_checker_protoc.approval_pb2 import Approval
from cloud.tools.contract_checker.contract_checker_protoc.config_pb2 import Config, Department, Person, Admin, Attachment


LOGGER = logging.getLogger('contract_checker')

# noinspection SpellCheckingInspection
DEFAULT_CONFIG = Config(
    planning=Department(
        name='Planning Department',
        staff_logins=('bbuvaev', 'mblagutin'),
        sla=1,
    ),
    legal=Department(
        name='Legal Department',
        email='cloud_contracts@yandex-team.ru',
        staff_department='yandex_legal_control_economic_9828',
        sla=2,
    ),
    sales=Department(
        name='Project transaction support group',
        email='docs-project@yandex-team.ru',
        # staff_logins=('vassilena', 'sevda'),
        staff_department='yandex_biz_com_supp_register_accounting_project',
        sla=2,
    ),
    manager=Person(
        sla=1,
    ),
    startrek_query='Queue: CLOUDDOCS Tags: !skip Status: "In Progress"',
    robot='robot-yc-business',
    administration=Admin(
        name="Document's administrators",
        lead='mashmellow',
        email='ops_tasks@yandex-team.ru',
    )
)

DEPARTMENTS_CACHE = {}


NotificationTarget = collections.namedtuple('NotificationTarget', ('id', 'summon'))


class NobodyAvailable(Exception):
    pass


class UnknownStage(Exception):
    pass


def is_approved(approval):
    return all((
        approval.planning.state,
        approval.legal.state,
        approval.sales.state,
        approval.manager.state,
    ))


def get_contract_type(dry_run, config, ticket):
    approval = Approval()
    counter = 0

    for component in ticket.components:
        if component.name in ('tender', 'внесение_правок', 'VAR_с_правками'):
            counter += 1

        if component.name in ('без_правок', 'VAR_без_правок', 'interco'):
            counter += 1

            approval.planning.state = True
            approval.planning.person = config.robot

            approval.legal.state = True
            approval.legal.person = config.robot

    if counter >= 2:
        message = (
            '{}: Mixed components, please check ticket components and leave any one of \'tender\', \'внесение_правок\', \'без_правок\' or none of them.'
        ).format(ticket.key)

        LOGGER.info('creating comment in "%s": %s', ticket.key, message)
        if not dry_run:
            ticket.comments.create(text=message, summonees=config.robot)

        raise ValueError(message)

    if not counter:
        for stage in ('planning', 'legal', 'manager'):
            approval_state = getattr(approval, stage)
            setattr(approval_state, 'state', True)
            setattr(approval_state, 'person', config.robot)

    return approval


def get_departments(staff_client, staff_login):
    global DEPARTMENTS_CACHE

    if staff_login not in DEPARTMENTS_CACHE:
        response = staff_client.get_person_departments(staff_login)

        try:
            departments = [
                department['department']['url'] for department in response['result'][0]['group']['ancestors']
            ]
            departments.append(response['result'][0]['group']['department']['url'])
            DEPARTMENTS_CACHE[staff_login] = departments

        except Exception as e:
            try:
                reason = response.reason
            except Exception as reason_exception:
                LOGGER.exception('failed to extract reason from Staff API response: %s', format_exception(reason_exception))
                reason = 'unknown reason'

            LOGGER.exception('failed to check get departments for "%s": %s: %s', staff_login, reason, format_exception(e))
            DEPARTMENTS_CACHE[staff_login] = []

    return DEPARTMENTS_CACHE[staff_login]


def check_approves(config, staff_client, ticket, approval):
    if len(ticket.votedBy) > 0:
        if not approval.planning.state:
            for person in ticket.votedBy:
                if (
                    (person.login in config.planning.staff_logins) or
                    (config.planning.staff_department and config.planning.staff_department in get_departments(staff_client, person.login))
                ):
                    approval.planning.state = True
                    approval.planning.person = person.login

        if not approval.legal.state:
            for person in ticket.votedBy:
                if (
                    (person.login in config.legal.staff_logins) or
                    (config.legal.staff_department and config.legal.staff_department in get_departments(staff_client, person.login))
                ):
                    approval.legal.state = True
                    approval.legal.person = person.login

        if not approval.sales.state:
            for person in ticket.votedBy:
                if (
                    (person.login in config.sales.staff_logins) or
                    (config.sales.staff_department and config.sales.staff_department in get_departments(staff_client, person.login))
                ):
                    approval.sales.state = True
                    approval.sales.person = person.login

        if not approval.manager.state:
            for person in ticket.votedBy:
                if config.manager.staff_login and person.login in config.manager.staff_login:
                    approval.manager.state = True
                    approval.manager.person = person.login

    return approval


def format_login(staff_login):
    if staff_login:
        return 'кто:{}'.format(staff_login)
    return ''


def create_message(approval, attachments):
    LOGGER.debug('creating message for approval:\n%s', approval)

    message_template = resource.find('/contract_checker/message.txt').decode('utf-8')

    if attachments:
        attachments_list = str()
        for attach in attachments:
            attachments_list += '- (({} {}))\n'.format(attach.url.decode('utf-8'), attach.name.decode('utf-8'))
    else:
        attachments_list = u'Нет прикрепленных документов'

    return message_template.format(
        approval.planning.state, format_login(approval.planning.person),
        approval.legal.state, format_login(approval.legal.person),
        approval.sales.state, format_login(approval.sales.person),
        approval.manager.state, format_login(approval.manager.person),
        u'согласован' if is_approved(approval) else u'!!не согласован!!',
        attachments_list,
    )


def find_comment_id(config, ticket, text):
    for comment in ticket.comments.get_all():
        if comment.createdBy.login == config.robot and comment.text.startswith(text):
            return comment.id


def update_status(dry_run, config, ticket, message):
    comment_id = find_comment_id(config, ticket, 'Статус')

    LOGGER.info(u'updating status for %s (comment #%s), message:\n%s', ticket.key, comment_id, message)
    if not dry_run:
        if comment_id:
            ticket.comments[comment_id].update(text=message)
        else:
            ticket.comments.create(text=message)


def check_update_date(comment):
    diff = BusinessHours(get_datetime(comment['updatedAt']), now()).days
    return diff >= 1


def is_deadline_exceeded(comment, stage):
    diff = BusinessHours(get_datetime(comment['createdAt']), now()).days
    return diff >= stage.sla


def select_staff_login(config, stage, staff_client):
    if isinstance(stage, Person):
        if stage.staff_login and staff_client.is_available(stage.staff_login):
            return NotificationTarget(id=stage.staff_login, summon=True), True

    elif isinstance(stage, Department):
        if stage.staff_logins:
            for login in stage.staff_logins:
                if staff_client.is_available(login):
                    return NotificationTarget(id=login, summon=True), True

            raise NobodyAvailable(stage)

        if stage.email:
            return NotificationTarget(id=stage.email, summon=True), False

        if stage.staff_department and stage.staff:
            return NotificationTarget(id=stage.staff, summon=False), True
        elif stage.email:
            return NotificationTarget(id=stage.email, summon=False), False

        raise NobodyAvailable(stage)

    else:
        raise UnknownStage(stage)

    LOGGER.warning('nobody available from stage:\n%s', stage)
    raise NobodyAvailable(stage)


def notify(dry_run, config, staff_client, ticket, stage, notify_during_business_hours, startrek_client, issue):
    if notify_during_business_hours:
        if not 9 < now().hour < 20:
            LOGGER.warning('it\'s not working hours (9:00 - 20:00), so notifications will not be sent')
            return

    notification_target, is_staff = select_staff_login(config=config, stage=stage, staff_client=staff_client)  # type: NotificationTarget

    comment = None
    comment_id = find_comment_id(config=config, ticket=ticket, text='кто:{}'.format(notification_target.id))
    if comment_id:
        comment = ticket.comments[comment_id]

    def _update_ticket(text, summonees):
        ticket = get_ticket(startrek_client, issue)
        if is_staff:
            if comment:
                comment.update(text=text, summonees=summonees)
            else:
                ticket.comments.create(text=text, summonees=summonees)
        else:
            if comment:
                comment.update(text=text, maillistSummonees=summonees)
            else:
                ticket.comments.create(text=text, maillistSummonees=summonees)

    should_update_ticket = (
        (comment is not None and check_update_date(comment)) or
        (comment is None)
    )

    if should_update_ticket:
        try:
            message = 'кто:{}, пожалуйста, посмотрите предложенный договор и вынесите свое решение.'.format(notification_target.id)
            LOGGER.info('%s: %s (comment #%s), message:\n%s', ticket.key, notification_target, comment_id, message)

            if not dry_run:
                _update_ticket(text=message, summonees=notification_target.id)
                ticket = get_ticket(startrek_client, issue)

        except NobodyAvailable:
            message = 'В данный момент никто из {} недоступен. Требуется ручное вмешательство в процесс согласования.'.format(stage.name)
            LOGGER.info(
                '%s: (comment #%s, summoning %s, setting tag "skip"), message:\n%s',
                ticket.key, comment_id, config.administration.email, message,
            )

            if not dry_run:
                _update_ticket(text=message, maillistSummonees=config.administration.email)
                ticket = get_ticket(startrek_client, issue)
                ticket.update(tags={'add': ['skip']})
                ticket = get_ticket(startrek_client, issue)

    if comment and is_deadline_exceeded(comment=comment, stage=stage):
        message = 'Срок согласования "{}" стадии подошел к концу, обратите внимание.'.format(stage.name)
        LOGGER.info(
            'notifying %s via %s (comment #%s, summoning %s), message:\n%s',
            config.administration.email, ticket.key, comment_id, config.administration.email, message,
        )

        if not dry_run:
            ticket.comments.create(text=message, maillistSummonees=config.administration.email)
            ticket = get_ticket(startrek_client, issue)
            ticket.update(tags={'add': ['skip']})
            ticket = get_ticket(startrek_client, issue)


def get_startrek_client(startrek_token):
    startrek_client = Startrek(
        useragent='Python script for Cloud business (cloud/tools/contract_checker)',
        base_url='https://st-api.yandex-team.ru/v2',
        token=startrek_token,
    )
    return startrek_client


def get_ticket(startrek_client, issue):
    return startrek_client.issues[issue]


def get_attachments(ticket):
    attachments = list()
    if ticket.attachments:
        for attach in ticket.attachments:
            attachments.append(Attachment(name=attach.name, url=attach.content))
    return attachments


def change_ticket_status(dry_run, startrek_client, issue, ticket_status, approval):
    ticket = get_ticket(startrek_client, issue)
    LOGGER.info('%s: change status to %s', ticket.key, ticket_status)
    if not dry_run and all((
        approval.planning.state,
        approval.legal.state,
        approval.sales.state,
        approval.manager.state,
    )):
        ticket = get_ticket(startrek_client, issue)
        ticket.transitions[ticket_status].execute()


def run(dry_run, config, startrek_token, staff_token, sleep=True, notify_during_business_hours=True):
    LOGGER.info('dry run: %s; notify during business hours: %s, running with config:\n%s', dry_run, notify_during_business_hours, config)

    if sleep:
        LOGGER.info('sleeping for 10 seconds, in case you need to change anything; add "--no-sleep" to avoid this')
        time.sleep(10)

    startrek_client = get_startrek_client(startrek_token)
    staff_client = StaffClient(oauth_token=staff_token)

    for issue in startrek_client.issues.find(config.startrek_query):
        issue = issue.key
        LOGGER.info('processing %s', issue)
        time.sleep(0.5)

        ticket = get_ticket(startrek_client, issue)
        LOGGER.debug('ticket:\n%s', ticket)

        ticket_config = Config()
        ticket_config.CopyFrom(config)
        ticket_config.manager.staff_login = ticket.createdBy.login
        LOGGER.debug('ticket config:\n%s', ticket_config)

        approval = get_contract_type(dry_run=dry_run, config=ticket_config, ticket=ticket)
        approval = check_approves(config=ticket_config, staff_client=staff_client, ticket=ticket, approval=approval)

        LOGGER.info('%s: approved=%s', ticket.key, is_approved(approval))

        if not is_approved(approval):
            if not approval.planning.state:
                notify(
                    dry_run=dry_run,
                    config=ticket_config,
                    staff_client=staff_client,
                    ticket=ticket,
                    stage=ticket_config.planning,
                    notify_during_business_hours=notify_during_business_hours,
                    startrek_client=startrek_client,
                    issue=issue,
                )
            elif not approval.legal.state:
                notify(
                    dry_run=dry_run,
                    config=ticket_config,
                    staff_client=staff_client,
                    ticket=ticket,
                    stage=ticket_config.legal,
                    notify_during_business_hours=notify_during_business_hours,
                    startrek_client=startrek_client,
                    issue=issue,
                )
            elif not approval.sales.state:
                notify(
                    dry_run=dry_run,
                    config=ticket_config,
                    staff_client=staff_client,
                    ticket=ticket,
                    stage=ticket_config.sales,
                    notify_during_business_hours=notify_during_business_hours,
                    startrek_client=startrek_client,
                    issue=issue,
                )
            elif not approval.manager.state:
                notify(
                    dry_run=dry_run,
                    config=ticket_config,
                    staff_client=staff_client,
                    ticket=ticket,
                    stage=ticket_config.manager,
                    notify_during_business_hours=notify_during_business_hours,
                    startrek_client=startrek_client,
                    issue=issue,
                )

        attachments = get_attachments(ticket)
        message = create_message(approval, attachments)
        update_status(dry_run=dry_run, config=ticket_config, ticket=ticket, message=message)
        change_ticket_status(dry_run=dry_run, startrek_client=startrek_client, issue=issue, ticket_status='docSign', approval=approval)


def main():
    configure_basic_logging('contract_checker', loggers={'contract_checker'})

    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--dry-run', action='store_true')
    parser.add_argument('--no-sleep', action='store_true')
    parser.add_argument('--startrek-token', required=True)
    parser.add_argument('--staff-token', required=True)
    parser.add_argument('-c', '--config', required=False)
    args = parser.parse_args()

    final_config = DEFAULT_CONFIG

    if args.config:
        try:
            with open(args.config) as fd:
                config_source = yaml.load(fd.read())

            parsed_config = Config()
            ParseDict(config_source, parsed_config)
            final_config.MergeFrom(parsed_config)

        except Exception as e:
            error_message = 'failed to parse config at "{}": {}'.format(args.config, format_exception(e))
            LOGGER.exception(error_message)
            raise RuntimeError(error_message)
    else:
        LOGGER.info('using default config')

    run(
        dry_run=args.dry_run,
        config=final_config,
        startrek_token=args.startrek_token,
        staff_token=args.staff_token,
        sleep=(not args.no_sleep),
    )


if __name__ == '__main__':
    main()

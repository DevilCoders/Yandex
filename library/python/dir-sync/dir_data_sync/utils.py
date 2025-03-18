# coding: utf-8
from logging import getLogger

from django.conf import settings
from django.db import transaction
from django.utils import timezone

from ylog.context import log_context

from .dir_client import DirClient, DirClientError
from .models import ChangeEvent, Organization, get_default_operating_mode
from .signals import organization_imported
from .logic import OrganizationSyncStatistics, make_sync_random_for_req_id
from .dao import get_organization, get_operating_mode


logger = getLogger(__name__)


@transaction.atomic
def __do_import(org_obj):
    dir_org_id = org_obj['id']
    name = org_obj['name']
    label = org_obj['label']
    lang = org_obj['language']
    plan = org_obj['subscription_plan']
    mode = get_operating_mode(plan) if plan else get_default_operating_mode()
    org = Organization.objects.create(
        dir_id=dir_org_id,
        name=name,
        label=label,
        lang=lang,
        mode=mode,
        status=Organization.ORG_STATUSES.enabled
    )
    event = ChangeEvent.objects.create(org=org)
    event = ChangeEvent.objects.select_for_update().get(id=event.id)
    logger.info('New organization: id="%s", dir_id="%s", name="%s", label="%s"', org.id, dir_org_id, name, label)

    revision = import_org_data(dir_org_id)

    organization_imported.send(sender=import_dir_organization, object=org_obj)

    event.last_pull_at = timezone.now()
    event.revision = revision
    event.save()
    return org


def import_dir_organization(org_obj):
    """
    Импортировать новую организацию из Директории.
    Вернуть None если импорт не удался.

    @rtype: Organization
    """
    if org_obj['is_blocked'] == True:
        # организация заблокирована в Директории
        logger.info('Can\'t import organization with dir_id={} because is it blocked in Directory.'.format(org_obj['id']))
        return

    organization = get_organization(dir_id=org_obj['id'])
    if organization:
        return organization

    try:
        with OrganizationSyncStatistics(org_obj['id']):
            return __do_import(org_obj)
    except Exception:
        # перехватываем все ошибки чтобы откатилась транзакция и код продолжил работать.
        logger.exception('Could not import organization %s', org_obj['id'])

    return None


def import_org_data(dir_org_id):
    """
    Импортировать данные о группах, департаментах и пользователях для новой организации через АПИ Директории.
    """
    from .signals import department_added, group_added, user_added

    dir_client = DirClient(
        sync_random=make_sync_random_for_req_id()
    )
    with log_context(req_id=dir_client.get_req_id(dir_org_id)):
        while True:
            deps, deps_revision = dir_client.get_departments(dir_org_id)
            groups, groups_revision = dir_client.get_groups(dir_org_id)
            users, users_revision = dir_client.get_users(dir_org_id)
            if deps_revision == groups_revision == users_revision:
                break
            else:
                with log_context(dir_org_id=dir_org_id, deps_revision=deps_revision,
                                 groups_revision=groups_revision, users_revision=users_revision):
                    logger.warn('Try to load data of organization with latest revision again')

        send_signals_for_obj_list(dir_org_id, _make_valid_dep_order_list(deps), department_added)
        send_signals_for_obj_list(dir_org_id, _make_valid_group_order_list(groups), group_added)
        send_signals_for_obj_list(dir_org_id, users, user_added)

        return deps_revision


def ensure_service_ready():
    """
    Уведомить Директорию о готовности организаций, которые уже импортированы, но по каким-то причинам не была отмечена
    в Директории.
    """

    dir_client = DirClient(
        sync_random=make_sync_random_for_req_id()
    )
    dir_org_ids = set(Organization.objects.values_list('dir_id', flat=True))
    dir_org_ids = [org['id'] for org in dir_client.get_organizations(**{"service.ready": False}) if str(org['id']) in dir_org_ids]
    for dir_org_id in dir_org_ids:
        with log_context(req_id=dir_client.get_req_id(dir_org_id)):
            try:
                dir_client.post_service_ready(
                    service_slug=settings.DIRSYNC_SERVICE_SLUG,
                    dir_org_id=dir_org_id,
                )
            except DirClientError:
                logger.exception('Failed to ensure service ready for organization "%s:', dir_org_id)


def send_signals_for_obj_list(dir_org_id, objects, signal):
    for object in objects:
        # добавляем dir_org_id в объект, так как его там нет, а нам надо знать в обработчике события какая это организация
        # отдельным параметром не добавляем, так как в уведомлениях объекты содержат dir_org_id и передаются с сигналом
        object['org_id'] = dir_org_id
        signal.send(
            sender=import_org_data,
            object=object,
            content=object,
        )


def _make_valid_dep_order_list(deps):
    # Упорядочить список департаментов так, чтобы родительские объекты были перед дочерними, чтобы обеспечить
    # последовательный импорт объектов с указанием связей между ними. Связь между департаментами в данных от Директории
    # представляется ссылкой на родительский объект: id -> parent_id
    obj_list = list()
    ids_list = set()

    for dep in deps:
        if not dep.get('parent'):
            # в первой итерации выбираем объекты, у которых нет родительских (вершины дерева)
            obj_list.append(dep)
            ids_list.add(dep['id'])

    while len(deps) > len(obj_list):
        for dep in deps:
            if dep['id'] not in ids_list and dep['parent']['id'] in ids_list:
                # во второй и последующей итерациях выбираем объекты последовательно: к уже выбранным родительским
                # объектам добавляем дочерние
                obj_list.append(dep)
                ids_list.add(dep['id'])

    return obj_list


def _make_valid_group_order_list(groups):
    # Упорядочить список групп так, чтобы дочерние объекты были перед родительскими. Хотя в группу могут быть включены
    # не только другие группы, но и пользователи, и департаменты, упорядочить надо только группы по связям
    # группа-группа, так как еще не существующая группа в связи является проблемой при последовательном импорте
    # объектов с указанием связей между ними. В данным про группу от Директории есть информация только про вложенные
    # в группу объекты с указанием их типа в списке members.
    obj_list = list()
    ids_list = set()

    for group in groups:
        if group['members_count'] == 0 or not any(obj['type'] == 'group' for obj in group['members']):
            # в первой итерации выбираем объекты, у которых нет дочерних объектов с типом 'группа' (листья дерева)
            obj_list.append(group)
            ids_list.add(group['id'])

    while len(groups) > len(obj_list):
        for group in groups:
            if (group['id'] not in ids_list and
                    [obj for obj in group['members'] if obj['type'] == 'group' and obj['object']['id'] in ids_list]):
                # во второй и последующей итерациях выбираем объекты последовательно: к уже выбранным дочерним
                # объектам-группам добавляем родительские
                obj_list.append(group)
                ids_list.add(group['id'])

    return obj_list

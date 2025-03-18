# coding: utf-8
from logging import getLogger

from django.dispatch import Signal, receiver
from django.conf import settings

from .logic import disable_organization, enable_organization


logger = getLogger(__name__)


# См. док-цию по формату описания событий тут - https://api-internal.directory.ws.yandex.net/docs/events.html

user_added = Signal(providing_args=['object', 'content'])

user_dismissed = Signal(providing_args=['object', 'content'])

user_property_changed = Signal(providing_args=['object', 'content'])

user_moved = Signal(providing_args=['object', 'content'])

# этот сигнал в настоящий момент не обрабатывается в приложении, эти изменения обрабатываются в user_moved
department_user_added = Signal(providing_args=['object', 'content'])

# этот сигнал в настоящий момент не обрабатывается в приложении, эти зменения обрабатываются в user_moved
department_user_deleted = Signal(providing_args=['object', 'content'])

department_property_changed = Signal(providing_args=['object', 'content'])

department_added = Signal(providing_args=['object', 'content'])

# этот сигнал в настоящий момент не обрабатывается в приложении, эти зменения обрабатываются в department_moved
department_department_added = Signal(providing_args=['object', 'content'])

# этот сигнал в настоящий момент не обрабатывается в приложении, эти зменения обрабатываются в department_moved
department_department_deleted = Signal(providing_args=['object', 'content'])

department_moved = Signal(providing_args=['object', 'content'])

department_deleted = Signal(providing_args=['object', 'content'])

group_added = Signal(providing_args=['object', 'content'])

group_deleted = Signal(providing_args=['object', 'content'])

group_property_changed = Signal(providing_args=['object', 'content'])

group_membership_changed = Signal(providing_args=['object', 'content'])

# этот сигнал в настоящий момент не обрабатывается в приложении
resource_grant_changed = Signal(providing_args=['object', 'content'])

# этот сигнал в настоящий момент не обрабатывается в приложении (изменения обрабатываются в group_membership_changed)
department_group_added = Signal(providing_args=['object', 'content'])

# этот сигнал в настоящий момент не обрабатывается в приложении (изменения обрабатываются в group_membership_changed)
group_group_added = Signal(providing_args=['object', 'content'])

# этот сигнал в настоящий момент не обрабатывается в приложении (изменения обрабатываются в group_membership_changed)
group_group_deleted = Signal(providing_args=['object', 'content'])

# этот сигнал в настоящий момент не обрабатывается в приложении (изменения обрабатываются в group_membership_changed)
department_group_deleted = Signal(providing_args=['object', 'content'])

# этот сигнал в настоящий момент не обрабатывается в приложении (вместо него - service_enabled)
organization_migrated = Signal(providing_args=['object', 'content'])

# этот сигнал в настоящий момент не обрабатывается в приложении (вместо него - service_enabled)
organization_added = Signal(providing_args=['object', 'content'])

# Сигнал о том, что данные новой организации заимпортированы в приложение
organization_imported = Signal(providing_args=['object'])

service_enabled = Signal(providing_args=['object', 'content'])

service_disabled = Signal(providing_args=['object', 'content'])

organization_deleted = Signal(providing_args=['object', 'content'])

organization_display_domain_changed = Signal(providing_args=['object', 'content'])

domain_master_changed = Signal(providing_args=['object', 'content'])

organization_subscription_plan_changed = Signal(providing_args=['object', 'content'])

# Сигналы о добавлении и снятии админских полномочий с сотрудника. Нужно использовать их, так как апдейты по
# служебной группе organization_admin не прилетают (WIKI-13406) и она будет всегда пустой
security_user_grant_organization_admin = Signal(providing_args=['object'])

security_user_revoke_organization_admin = Signal(providing_args=['object'])

security_user_blocked = Signal(providing_args=['object'])

security_user_unblocked = Signal(providing_args=['object'])

# Сигналы о прочих модификациях групп. Раньше вместо них прилетали group_membership_changed
user_group_added = Signal(providing_args=['object', 'content'])

user_group_deleted = Signal(providing_args=['object', 'content'])

@receiver(service_enabled)
def process_service_enabled(sender, object, content, **kwargs):
    if content['slug'] != settings.DIRSYNC_SERVICE_SLUG:
        return
    dir_org_id = str(object['org_id'])
    enable_organization(dir_org_id)


@receiver([service_disabled, organization_deleted])
def process_service_disabled(sender, object, content, **kwargs):
    if content['slug'] != settings.DIRSYNC_SERVICE_SLUG:
        return
    dir_org_id = str(object['org_id'])
    disable_organization(dir_org_id)

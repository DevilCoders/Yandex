# coding: utf-8
import yenv
from dir_data_sync import OPERATING_MODE_NAMES

# OAuth-токен для авторизации в АПИ Директории, обязателен если не используется TVM2
DIRSYNC_OAUTH_TOKEN = None

# При True походы в директорию будут производиться с TVM2 тикетами
DIRSYNC_USE_TVM2 = False

# Id собственного клиента TVM2, секрет и blackbox окружение, обязательны если используется TVM2
DIRSYNC_TVM2_CLIENT_ID = None
DIRSYNC_TVM2_SECRET = None
# from ticket_parser2.api.v1 import BlackboxClientId
# Должен быть вида BlackboxClientId.<окружение>
DIRSYNC_TVM2_BLACKBOX_CLIENT = None

# Значение False даёт "холостую работу" контекстного менеджера
DIRSYNC_IS_BUSINESS = False

# Значение True позволяет игнорировать отсутствие Организации в тестах
DIRSYNC_IS_TESTS = False

# Слаг сервиса (в рамках Коннекта), обязателен при синхронизации
DIRSYNC_SERVICE_SLUG = None

# Токен, с которым приходит Директория в сервис, обязателен при синхронизации
DIRSYNC_DIR_TOKEN = None

DIRSYNC_DIR_TVM2_CLIENTS = {
    'production': '2000205',
    'preprod': '2000205',
    'testing': '2000204',
    'unstable': '2000204',
    'development': '2000204',
}

DIRSYNC_DIR_TVM2_CLIENT_ID = yenv.choose_key_by_type(DIRSYNC_DIR_TVM2_CLIENTS)

if DIRSYNC_IS_TESTS:
    DIRSYNC_DIR_API_HOST = 'https://api-internal-test.directory.ws.yandex.net'
else:
    DIRSYNC_DIR_API_HOST = 'https://api-internal.directory.ws.yandex.net'

# Значения ограничений по-умолчанию для различных режимов работы организации.
DIRSYNC_DEFAULT_OPERATING_MODE_LIMITS = {}

# Список значений по-умолчанию для сбора статистики по организации.
DIRSYNC_DEFAULT_ORG_STATISTICS = {}

# Значения ограничений для списка режимов работы организации, где ключ - это имя режима работы,
# а значение - это словарь с набором ограничений для этого режима
DIRSYNC_OPERATING_MODE_LIMITS_BY_MODE_NAME = {
    OPERATING_MODE_NAMES.free: DIRSYNC_DEFAULT_OPERATING_MODE_LIMITS,
    OPERATING_MODE_NAMES.paid: DIRSYNC_DEFAULT_OPERATING_MODE_LIMITS,
}

DIRSYNC_FIELDS_ORGANIZATION = ','.join([
    'id',
    'label',
    'name',
    'language',
    'subscription_plan',
    'is_blocked',
])

# Поля, которые будут запрашиваться у Директории для пользователей, групп, департаментов и
# в итоге попадут в обработчики сигналов. В качестве дефолтных указан полный набор полей
# для каждой сущности на момент версии API v7. Полный набор полей нужен,
# чтобы обеспечить обратную совместимость при переходе с API v2 на v7, т.к.
# в v7 без fields в ручках /users/, /deparments/, /groups/ стал возвращаться только id.
DIRSYNC_FIELDS_USER = ','.join([
    'id',
    'org_id',
    'login',
    'email',
    'department_id',
    'department',
    'groups',
    'name',
    'gender',
    'position',
    'about',
    'birthday',
    'contacts',
    'aliases',
    'nickname',
    'external_id',
    'is_dismissed',
    'is_admin',
    'is_robot',
    'service_slug',
    'is_enabled'
])
DIRSYNC_FIELDS_DEPARTMENT = ','.join([
    'id',
    'org_id',
    'parent_id',
    'name',
    'path',
    'description',
    'heads_group_id',
    'label',
    'email',
    'removed',
    'aliases',
    'external_id',
    'created',
    'parents',
    'parent',
    'head',
    'members_count',
])
DIRSYNC_FIELDS_GROUP = ','.join([
    'id',
    'org_id',
    'admins',
    'all_users',
    'author',
    'name',
    'description',
    'label',
    'email',
    'removed',
    'aliases',
    'external_id',
    'created',
    'members',
    'member_of',
    'members_count',
    'type',
])

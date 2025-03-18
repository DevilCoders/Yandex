# coding: utf-8
'''
Настройки для работы с яндексовой авторизацией.

Некоторые из них обязательны, поэтому при подключении авторизации
нужно импортировать этот файл в настройки проекта целиком, а потом
уже можно что-то заменять.
'''

from tvmauth import BlackboxTvmId as BlackboxClientId


YAUTH_AVAILABLE_TLDS = [
    '.az', '.by', '.com',
    '.co.il', '.com.am', '.com.ge', '.com.tr',
    '.ee', '.eu', '.fi', '.fr', '.kg', '.kz', '.lt', '.lv',
    '.md', '.pl', '.ru', '.tj', '.tm', '.ua', '.uz',
]

YAUTH_PASSPORT_URLS = {
    'desktop': {
        'create': 'https://passport.yandex.ru/auth?retpath=',
        'create_secure': 'https://passport.yandex.ru/auth/secure/?retpath=',
        'create_guard': 'https://passport.yandex.ru/auth/guard?retpath=',
        'delete': 'https://passport.yandex.ru/passport?mode=logout&retpath=',
        'refresh': 'https://passport.yandex.ru/auth/update?retpath=',
        'verify': 'https://passport.yandex.ru/auth/verify?retpath=',
        'register': 'https://passport.yandex.ru/registration&retpath=',
        'passport_account': 'https://passport.yandex.ru/profile',
        'subscribe': 'https://passport.yandex.ru/passport?mode=subscribe&from=%s&retpath=',
        'admsubscribe': 'http://passport-internal.yandex.ru/passport?mode=admsubscribe&from=%(slug)s&uid=%(uid)s',
        'passport_domain': 'passport.yandex.ru',
    },
    'mobile': {
        'create': 'https://pda-passport.yandex.ru/auth?retpath=',
        'create_secure': 'https://pda-passport.yandex.ru/auth/secure/?retpath=',
        'create_guard': 'https://pda-passport.yandex.ru/auth/guard?retpath=',
        'delete': 'https://pda-passport.yandex.ru/passport?mode=logout&retpath=',
        'refresh': '',
        'verify': '',
        'register': 'https://pda-passport.yandex.ru/registration&retpath=',
        'passport_account': 'https://pda-passport.yandex.ru/profile',
        'subscribe': 'https://pda-passport.yandex.ru/passport?mode=subscribe&from=%s&retpath=',
        'admsubscribe': '',
        'passport_domain': 'pda-passport.yandex.ru',
    },
    'intranet': {
        'create': 'https://passport.yandex-team.ru/auth?retpath=',
        'create_secure': 'https://passport.yandex-team.ru/auth/secure/?retpath=',
        'create_guard': 'https://passport.yandex-team.ru/auth/guard?retpath=',
        'delete': 'https://passport.yandex-team.ru/passport?mode=logout&retpath=',
        'refresh': 'https://passport.yandex-team.ru/auth/update?retpath=',
        'verify': 'https://passport.yandex-team.ru/auth/verify?retpath=',
        'passport_account': 'https://passport.yandex-team.ru/profile',
        'subscribe': 'https://passport.yandex-team.ru/passport?mode=subscribe&from=%s&retpath=',
        'admsubscribe': 'http://passport-internal.yandex-team.ru/passport?mode=admsubscribe&from=%(slug)s&uid=%(uid)s',
        'passport_domain': 'passport.yandex-team.ru',
    },
    'intranet-testing': {
        'create': 'https://passport-test.yandex-team.ru/auth?retpath=',
        'create_secure': 'https://passport-test.yandex-team.ru/auth/secure/?retpath=',
        'create_guard': 'https://passport-test.yandex-team.ru/auth/guard?retpath=',
        'delete': 'https://passport-test.yandex-team.ru/passport?mode=logout&retpath=',
        'refresh': 'https://passport-test.yandex-team.ru/auth/update?retpath=',
        'verify': 'https://passport-test.yandex-team.ru/auth/verify?retpath=',
        'passport_account': 'https://passport-test.yandex-team.ru/profile',
        'subscribe': 'https://passport-test.yandex-team.ru/passport?mode=subscribe&from=%s&retpath=',
        'admsubscribe': 'http://passport-test-internal.yandex-team.ru/passport?mode=admsubscribe&from=%(slug)s&uid=%(uid)s',
        'passport_domain': 'passport-test.yandex-team.ru',
    },
}

YAUTH_PASSPORT_URLS['mobile']['refresh'] = YAUTH_PASSPORT_URLS['desktop']['refresh']
YAUTH_PASSPORT_URLS['mobile']['admsubscribe'] = YAUTH_PASSPORT_URLS['desktop']['admsubscribe']

# Параметр, на основе которого генерируются пути к Паспорту
YAUTH_TYPE = None  # автоопределение

# Имя сервиса в Паспорте. Требуется для прямой подписки пользователя на сервис
YAUTH_PASSPORT_SERVICE_SLUG = ''

# Имя куки для хранения id сессии. Менять не нужно.
YAUTH_SESSIONID_COOKIE_NAME = 'Session_id'

# Имя дополнительной куки для хранения id сессии, выставляемой только по HTTPS. Менять не нужно.
YAUTH_SESSIONID2_COOKIE_NAME = 'sessionid2'

# Имя куки-оберега (для дополнительной защиты отдельных поддоменов). Менять не нужно.
YAUTH_SESSGUARD_COOKIE_NAME = 'sessguard'

# Признак, выставляемый сервисом, если наличие дополнительной куки sessionid2 при соединении по HTTPS
# является обязательным.
YAUTH_SESSIONID2_REQUIRED = False

# Имя куки, наличие которой проверяет паспорт, чтобы проверить работу кук.
YAUTH_COOKIE_CHECK_COOKIE_NAME = 'Cookie_check'

# Надо ли подновлять все еще валидную куку, когда об этом сообщает Паспорт
YAUTH_REFRESH_SESSION = True

# Надо ли неявно создавать локальные для проекта профили при их отсутствии.
YAUTH_CREATE_PROFILE_ON_ACCESS = True

# Нужно ли создавать User при обращении к request.user при его отсутствии
YAUTH_CREATE_USER_ON_ACCESS = False

# Поле яндексового пользователя, которое будет ставится в соответствие
# USERNAME_FIELD джанговской модели.
YAUTH_YAUSER_USERNAME_FIELD = 'login'

# Дополнительные поля, которые будут проставлены
# джанго пользователю при создании
YAUTH_USER_EXTRA_FIELDS = (
    # ('login', 'username'),
)

# Дополнительные профили для яндексового пользователя.
# Это tuple строк в виде 'app.Model' -- моделей пользовательских
# профайлов нужных в проекте. Каждая модель должна иметь поле
# 'yandex_uid' и уметь создаваться без других дополнительных полей.
YAUTH_YAUSER_PROFILES = (
    # 'app.Model',
)

# Группа настроек для использования вместе с ЧЯ
import blackbox

# Кастомный класс для работы с ЧЯ.
# Необходим для инициализации классов-наследников blackbox.BaseBlackbox
# c нестандартными настройками, либо для замены ЧЯ на Mock-объект.
YAUTH_BLACKBOX_INSTANCE = None

# Поля, которые нужно дополнительно запрашивать у Паспорта при авторизации.
# На эти поля у проекта должны быть явно прописаны права у администраторов Паспорта.
YAUTH_PASSPORT_FIELDS = [blackbox.FIELD_LOGIN]

# Разрешать вход в админку Яндекс-пользователям с логинами, совпадающими с логинами
# заведенных в админке staff-пользователей
YAUTH_USE_NATIVE_USER = False

# Дополнительные параметры для запроса в blackbox.
# Например emails='getall', чтобы получить список адресов пользователя
YAUTH_BLACKBOX_PARAMS = {'emails': 'getdefault'}

# Использовать или нет django.contrib.sites фреймворк
YAUTH_USE_SITES = True

YAUTH_ANTI_REPEAT_PARAM = '_yauth'

YAUTH_CREATE_PROFILE_VIEW = 'yauth-create-profile'

YAUTH_CREATION_REDIRECT = False

YAUTH_CREATION_REDIRECT_METHODS = ('GET', 'HEAD')

# Делать request.user анонимным, если не нашли соответствующего
# для yauser в базе
YAUTH_ANONYMOUS_ON_MISSING = False

# Использовать OAuth при наличии правильного заголовка Authorization
YAUTH_USE_OAUTH = True

# Логин пользователя, либо словарь с ключами
# uid, login, default_email и т.п.
# для атрибутов тестового yauser
YAUTH_TEST_USER = None

# id тестового oauth приложения, либо словарь с ключами
# id, name, home_page, icon, uid, ctime, issue_time, expire_time, scope
YAUTH_TEST_APPLICATION = None

# Регулярка префикса который используется в заголовке Authorization
# Authorization: <prefix> <token>
YAUTH_OAUTH_HEADER_PATTERN_PREFIX = r'OAuth|Bearer'

# Последовательность HTTP заголовков, проверяемых в указанном
# порядке при выяснении реального IP пользователя.
YAUTH_REAL_IP_HEADERS = (
    'HTTP_X_REAL_IP',
    'HTTP_X_FORWARDED_FOR',
    'REMOTE_ADDR',
)

# Указывает, какие scopes дают возможность аутентифицироваться через oauth.
# Если пусто, проверка scopes отключена
YAUTH_OAUTH_AUTHORIZATION_SCOPES = []

# Имя заголовка, из которого будет браться хост для отправки в BB,
# вместо стандартных заголовков Host и X-Forwarded-Host.
YAUTH_HOST_HEADER = None

# tvm client id приложения (из tvm.yandex-team.ru)
# Отличное от None значение включает получение tvm-тикета из blackbox при
# авторизации. В этом случае должны быть также заданы настройки
# YAUTH_BLACKBOX_CONSUMER и YAUTH_TVM_CLIENT_SECRET
YAUTH_TVM_CLIENT_ID = None

# tvm client secret приложения (из tvm.yandex-team.ru)
# используется только при заданном YAUTH_TVM_CLIENT_ID
YAUTH_TVM_CLIENT_SECRET = None

# название потребителя (указанное в Грантушке)
# используется только при заданном YAUTH_TVM_CLIENT_ID
YAUTH_BLACKBOX_CONSUMER = None

# путь до поддержимаемых на проекте механизмов аутентификации
# реализующий класс должен иметь имя Mechanism
# Механизм TVM рекомендуется указывать первым так как
# это оффлайн механизм авторизации
YAUTH_MECHANISMS = [
    'django_yauth.authentication_mechanisms.cookie',
]

# Название заголовков, которые используются в TVM
YAUTH_TVM2_USER_HEADER = 'HTTP_X_YA_USER_TICKET'
YAUTH_TVM2_SERVICE_HEADER = 'HTTP_X_YA_SERVICE_TICKET'

# Список клиентов (client_id), которым можно приходить в сервис
# в будущем эта настройка будет удалена в пользу работы через ABC
YAUTH_TVM2_ALLOWED_CLIENT_IDS = []

# Значение client_id выданное при регистрации приложения в tvm
YAUTH_TVM2_CLIENT_ID = None

# Значение secret выданное при регистрации приложения в tvm
YAUTH_TVM2_SECRET = None

YAUTH_TVM2_API_URL = 'https://tvm-api.yandex.net'

YAUTH_TVM2_BLACKBOX_MAP = {
    'blackbox.yandex.net': BlackboxClientId.Prod,
    'blackbox-rc.yandex.net': BlackboxClientId.Prod,
    'blackbox-mimino.yandex.net': BlackboxClientId.Mimino,
    'blackbox.yandex-team.ru': BlackboxClientId.ProdYateam,
    'pass-test.yandex.ru': BlackboxClientId.Test,
    'blackbox-test.yandex-team.ru': BlackboxClientId.TestYateam,
}

# Позволяет задать свое окружение Blackbox для использования
# в UserContext механизма TVM и для получения персонализированных
# тикетов, в случае отсутствия нужного
# окружения в YAUTH_TVM2_BLACKBOX_MAP
# Должен быть вида BlackboxClientId.TestYateam
YAUTH_TVM2_BLACKBOX_CLIENT = None

# Определяет будет ли передаваться TVM2
# сервисный тикет при запросах в Blackbox
# при True необходимо задать настройки
# YAUTH_TVM2_CLIENT_ID и YAUTH_TVM2_SECRET
YAUTH_USE_TVM2_FOR_BLACKBOX = False


# Если True - при авторизации по пользовательскому
# TVM2 тикету - будет автоматически сделан запрос в ББ
# для получения данных пользователя из YAUTH_PASSPORT_FIELDS
YAUTH_TVM2_GET_USER_INFO = False

# Не работает без YAUTH_TVM2_GET_USER_INFO=True
# Если True - для получения данных пользователя используется сугубо blackbox.user_ticket
# https://docs.yandex-team.ru/blackbox/methods/user_ticket
YAUTH_TVM2_USE_USER_TICKET_FOR_USER_INFO = False

# Если True - при авторизации по session_id/oauth
# из блекбокса будет автоматически получен пользовательский тикет
# получить его можно из атрибута raw_user_ticket
# для работы так же нужно выставить YAUTH_USE_TVM2_FOR_BLACKBOX в True
# так как блекбокс не отдаст пользовательский тикет если в запросе нет
# сервисного
YAUTH_TVM2_GET_USER_TICKET = False

# None или строка пути до папки в которой tvmauth будет хранить кеш
# нужно для повышения надежности при работе с tvm и снижении
# нагрузки на апи tvm, подробнее про этот параметр
# https://a.yandex-team.ru/arc/trunk/arcadia/library/python/tvmauth/tvmauth/__init__.py?rev=r7887233#L325
YAUTH_TVM2_DISK_CACHE_DIR = None


# Включает экспериментальную фичу (tirole)
YAUTH_TVM2_FETCH_ROLES_FOR_IDM_SYSTEM = None
YAUTH_TVM2_TIROLE_ENV = 'production'
# Если True - будет проверка на наличие у src в сервис тикете
# хоть какой-то роли в системе YAUTH_TVM2_FETCH_ROLES_FOR_IDM_SYSTEM
# если такой нет - авторизация будет не успешной, без каких-либо
# дополнительных проверок с вашей стороны
YAUTH_TVM2_TIROLE_CHECK_SRC = False
# Аналогичная настройке выше, только для UserTicket'ов - будет проверять
# наличие хоть каких-либо прав в системе у пользователя при парсинге тикета
YAUTH_TVM2_TIROLE_CHECK_UID = False

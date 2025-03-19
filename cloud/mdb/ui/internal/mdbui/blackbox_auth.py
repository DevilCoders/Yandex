from blackboxer import Blackbox
from .tvm import TVMBlackbox
from django.conf import settings
from django.contrib.auth.middleware import AuthenticationMiddleware

INSTANCES = {}


class User:
    def __init__(self, username):
        self.username = username
        self.is_active = True
        self.is_staff = True
        self.is_superuser = True
        self.is_authenticated = True
        self.pk = 1

    def has_perm(self, perm, obj=None):
        return True

    def has_perms(self, perm_list, obj=None):
        return True

    def has_module_perms(self, app_label):
        return True

    def get_username(self):
        return self.username

    def __int__(self):
        return 1


class AnonUser:
    def __init__(self):
        self.username = ''
        self.is_active = False
        self.is_staff = False
        self.is_superuser = False
        self.is_authenticated = False

    def has_perm(self, perm, obj=None):
        return False

    def has_perms(self, perm_list, obj=None):
        return False

    def has_module_perms(self, app_label):
        return False

    def get_username(self):
        return ''


def ticket_generator():
    tvm = INSTANCES['tvm']
    client_id = INSTANCES['blackbox_client_id'].value
    tickets = tvm.get_service_tickets(client_id)
    return tickets[client_id]


class BlackboxAuthMiddleware(AuthenticationMiddleware):
    def process_request(self, request):

        if not settings.BLACKBOX_URL:
            request.user = User('MDB_USER')
            return

        request.user = AnonUser()

        if 'blackbox' not in INSTANCES:
            url = settings.BLACKBOX_URL
            if settings.USE_TVM:
                import tvm2
                from ticket_parser2_py3.api.v1 import BlackboxClientId

                INSTANCES['blackbox_client_id'] = getattr(BlackboxClientId, settings.TVM_BLACKBOX_ENV)
                INSTANCES['tvm'] = tvm2.TVM2(
                    client_id=settings.TVM_CLIENT_ID,
                    secret=settings.TVM_SECRET,
                    blackbox_client=INSTANCES['blackbox_client_id'],
                    destinations=(INSTANCES['blackbox_client_id'].value,),
                )
                INSTANCES['blackbox'] = TVMBlackbox(ticket_generator, url=url)
            else:
                INSTANCES['blackbox'] = Blackbox(url=url)

        blackbox = INSTANCES['blackbox']

        if 'HTTP_X_FORWARDED_FOR' in request.META:
            address = request.META.get('HTTP_X_FORWARDED_FOR')
        else:
            address = request.META.get('REMOTE_ADDR')

        sessionid = request.COOKIES.get('Session_id')
        if sessionid:
            result = blackbox.sessionid(address, sessionid, settings.BASE_HOST)
        else:
            token = request.META.get('HTTP_AUTHORIZATION')
            if not token:
                return

            token = token.replace('OAuth ', '')
            result = blackbox.oauth(userip=address, oauth_token=token)

        if not result.get('status', {}).get('value', '') == 'VALID':
            return

        if result['login'] in settings.ALLOWED_LOGINS:
            request.user = User(result['login'])

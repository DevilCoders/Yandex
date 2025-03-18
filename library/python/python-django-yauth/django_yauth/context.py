# -*- coding:utf-8 -*-

from django_yauth.util import (get_current_url, get_passport_url, get_passport_host,
                               get_setting, get_yauth_type)


def yauth(request):
    '''
    Процессор контекста с данными Яндекс-авторизацией.
    '''

    yauth_type = get_yauth_type(request)

    def yu():
        """Защита от CSRF - PASSP-1879"""
        yandexuid = request.COOKIES.get('yandexuid')
        if yandexuid:
            return '&yu=%s' % yandexuid
        return ''

    context = {
        'yauser': request.yauser,
        'login_url': get_passport_url('create', yauth_type, request, retpath=True),
        'logout_url': get_passport_url('delete', yauth_type, request, retpath=True) + yu(),
        'account_url': get_passport_url('passport_account', yauth_type, request),
        'passport_account_url': get_passport_url('passport_account', yauth_type, request),
        'current_url': get_current_url(request),
        'passport_host': get_passport_host(request),
    }

    # внутренний паспорт не умеет register
    if yauth_type not in ['intranet', 'intranet-testing']:
        register_url = get_passport_url('register', yauth_type, request, retpath=True)
        slug = get_setting(['PASSPORT_SERVICE_SLUG', 'YAUTH_PASSPORT_SERVICE_SLUG'])

        if slug:
            register_url += ('&msg=' + slug)
        context['register_url'] = register_url
    return context

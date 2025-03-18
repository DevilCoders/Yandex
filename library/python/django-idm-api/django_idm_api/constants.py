# coding: utf-8
from __future__ import unicode_literals

import attr

from django.conf import settings

_CN = {
    'testing': 'idm.test.yandex-team.ru',
    'production': 'idm.yandex-team.ru',
}[settings.IDM_INSTANCE]

IDM_CERT_SUBJECTS = (
    '/C=RU/ST=Moscow/L=Moscow/O=Yandex LLC/OU=ITO/CN={CN}/emailAddress=pki@yandex-team.ru'.format(CN=_CN),
    'emailAddress=pki@yandex-team.ru,CN={CN},OU=ITO,O=Yandex LLC,L=Moscow,ST=Moscow,C=RU'.format(CN=_CN),
)
IDM_CERT_ISSUERS = (
    '/DC=ru/DC=yandex/DC=ld/CN=YandexInternalCA',
    'CN=YandexInternalCA,DC=ld,DC=yandex,DC=ru',
)


class SUBJECT_TYPES(object):
    USER = 'user'
    TVM_APP = 'tvm_app'
    ORGANIZATION = 'organization'

    CHOICES = (
        (USER, USER),
        (TVM_APP, TVM_APP),
        (ORGANIZATION, ORGANIZATION),
    )

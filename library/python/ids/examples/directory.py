# coding: utf-8
from ids.registry import registry


directory_token = 'qaeb2ChDYeRfhgh4Ahji3uufaubs4riTAehne6dfb'  # fake


users_repo = registry.get_repository(
    service='directory',
    resource_type='user',
    user_agent='myservice',
    token=directory_token,
)


# полный список возможных параметров для фильтрации можно увидеть тут
# https://api.directory.ws.yandex.ru/docs/playground.html#spisok-sotrudnikov
#
# Параметры, передающиеся в заголовках (X-Org-ID, X-UID, X-USER-IP) передаются
# в lookup с именами x_org_id, x_uid, x_user_ip.

users = users_repo.getiter(
    {
        'x_org_id': 1,
    }
)
for user in users:
    print user


users = users_repo.getiter(
    {
        'x_org_id': 1,
        'nickname': 'user1,user2',
    }
)
for user in users:
    print user


# Пример данных реального человека
#     {
#       "birthday": "1976-04-29",
#       "is_admin": false,
#       "groups": [
#         {
#           "email": null,
#           "members_count": 42,
#           "name": {
#             "ru": "Администратор организации",
#             "en": "Organization administrator"
#           },
#           "created": "2016-05-24T08:52:25.998293Z",
#           "external_id": null,
#           "author_id": null,
#           "label": null,
#           "type": "organization_admin",
#           "id": 2,
#           "description": {
#             "ru": "",
#             "en": ""
#           }
#         },
#         {
#           "email": "party@cronies.ru",
#           "members_count": 210,
#           "name": {
#             "ru": "Новогодний корпоратив 2022"
#           },
#           "created": "2016-08-24T08:54:44.403007Z",
#           "external_id": null,
#           "author_id": null,
#           "label": "party",
#           "type": "generic",
#           "id": 94,
#           "description": {
#             "ru": ""
#           }
#         },
#         {
#           "email": "wiki-test-group6@cronies.ru",
#           "members_count": 1,
#           "name": {
#             "ru": "wiki-test-group6"
#           },
#           "created": "2016-09-30T11:35:54.692487Z",
#           "external_id": null,
#           "author_id": null,
#           "label": "wiki-test-group6",
#           "type": "generic",
#           "id": 121,
#           "description": {
#             "ru": "wiki-test-group6"
#           }
#         },
#         {
#           "email": "wiki-test-group8@cronies.ru",
#           "members_count": 2,
#           "name": {
#             "ru": "wiki-test-group8"
#           },
#           "created": "2016-10-03T09:48:49.701240Z",
#           "external_id": null,
#           "author_id": null,
#           "label": "wiki-test-group8",
#           "type": "generic",
#           "id": 129,
#           "description": {
#             "ru": ""
#           }
#         },
#         {
#           "email": "wiki-test-group10@cronies.ru",
#           "members_count": 1,
#           "name": {
#             "ru": "wiki-test-group101"
#           },
#           "created": "2016-10-11T14:08:04.120281Z",
#           "external_id": null,
#           "author_id": null,
#           "label": "wiki-test-group10",
#           "type": "generic",
#           "id": 133,
#           "description": {
#             "ru": "wiki-test-group10"
#           }
#         },
#         {
#           "email": "test-staff@cronies.ru",
#           "members_count": 210,
#           "name": {
#             "ru": "Просто тестовая команда"
#           },
#           "created": "2016-10-24T15:59:30.988620Z",
#           "external_id": null,
#           "author_id": null,
#           "label": "test-staff",
#           "type": "generic",
#           "id": 145,
#           "description": {
#             "ru": ""
#           }
#         },
#         {
#           "email": "wiki_super_team@cronies.ru",
#           "members_count": 1,
#           "name": {
#             "ru": "wiki super team"
#           },
#           "created": "2016-11-08T15:40:51.730127Z",
#           "external_id": null,
#           "author_id": null,
#           "label": "wiki_super_team",
#           "type": "generic",
#           "id": 150,
#           "description": {
#             "ru": ""
#           }
#         },
#         {
#           "email": "zzz@cronies.ru",
#           "members_count": 1,
#           "name": {
#             "ru": "000"
#           },
#           "created": "2016-11-08T18:49:50.375742Z",
#           "external_id": null,
#           "author_id": null,
#           "label": "zzz",
#           "type": "generic",
#           "id": 151,
#           "description": {
#             "ru": ""
#           }
#         },
#         {
#           "email": "bro@cronies.ru",
#           "members_count": 1,
#           "name": {
#             "ru": "Разработчики браузера"
#           },
#           "created": "2016-11-09T12:59:07.811558Z",
#           "external_id": null,
#           "author_id": null,
#           "label": "bro",
#           "type": "generic",
#           "id": 153,
#           "description": {
#             "ru": ""
#           }
#         }
#       ],
#       "is_dismissed": false,
#       "nickname": "elisei",
#       "id": 1130000021881450,
#       "aliases": [],
#       "about": null,
#       "name": {
#         "middle": {
#           "ru": ""
#         },
#         "last": {
#           "ru": "Микерин"
#         },
#         "first": {
#           "ru": "Алексей"
#         }
#       },
#       "contacts": [
#         {
#           "synthetic": true,
#           "alias": false,
#           "main": true,
#           "type": "email",
#           "value": "elisei@cronies.ru"
#         }
#       ],
#       "gender": "male",
#       "department": {
#         "members_count": null,
#         "description": null,
#         "parent": null,
#         "label": null,
#         "id": 1,
#         "external_id": null,
#         "email": null,
#         "name": {
#           "ru": "Все сотрудники",
#           "en": "All employees"
#         }
#       },
#       "position": {
#         "ru": ""
#       },
#       "login": "elisei",
#       "external_id": null,
#       "email": "elisei@cronies.ru"
#     }


org_repo = registry.get_repository(
    service='directory',
    resource_type='organization',
    user_agent='forms',
    tvm_ticket='...',
)
print org_repo.get_one({
    'x_uid': '...',
    'x_user_ip': '...',
    'fields': 'id,name,label',
})
# {u'label': u'wikiteam', u'id': 728, u'name': u'Wikiteam'}

# coding: utf-8
import logging

from ids.registry import registry


logging.basicConfig(level=logging.DEBUG)

TOKEN = 'cce8de3df9594e008681106e39fefc80'

# Люди
persons = registry.get_repository('staff', 'person', user_agent='myservice', oauth_token=TOKEN)

person = persons.get_one(lookup={'login': 'kukutz'})

print person['uid']


# Группы
groups = registry.get_repository('staff', 'group', user_agent='myservice', oauth_token=TOKEN)

dep_groups = groups.get(lookup={
    'type': 'department',
    'ancestors.url': 'yandex_infra_internal',
})

print [group['url'] for group in dep_groups]


# Членства в группах
memberships = registry.get_repository(
    'staff', 'groupmembership', user_agent='myservice', oauth_token=TOKEN)

tools_members = memberships.get({
    'group.id': 203,
    'person.official.is_dismissed': False,
})

for member in tools_members:
    print member['person']['name']['first']['ru']


# Организации
organizations = registry.get_repository(
    'staff', 'organization', user_agent='myservice', oauth_token=TOKEN)

new_orgs = organizations.get(lookup={
    '_query': 'created_at > "2011-05-24T14:50:02+00:00"'
})

for org in new_orgs:
    print org['name']


# Должности
positions = registry.get_repository('staff', 'position', user_agent='myservice', oauth_token=TOKEN)

position = positions.get_one(lookup={'id': '62'})

print position['name']['ru']


# Профессии
occupations = registry.get_repository('staff', 'occupation', user_agent='myservice', oauth_token=TOKEN)

occupation = occupations.get_one(lookup={'id': 'AcademicCoordinator'})

print occupation['description']['en']


# Офисы
offices = registry.get_repository('staff', 'office', user_agent='myservice', oauth_token=TOKEN)

ru_offices = offices.get(lookup={'city.country.name.en': 'Russia'})

for office in ru_offices:
    print office['name']['en']


# Столы
tables = registry.get_repository('staff', 'table', user_agent='myservice', oauth_token=TOKEN)

my_table = tables.get_one(lookup={'id': '1260'})

print my_table['number']


# Помещения
rooms = registry.get_repository('staff', 'room', user_agent='myservice', oauth_token=TOKEN)

msk_rooms = rooms.get(lookup={
    'floor.office.name.en': 'Moscow, BC Morozov',
    'floor.number': 5,
})

for room in msk_rooms:
    print room['name']['display']


# Оборудование
equipment = registry.get_repository('staff', 'equipment', user_agent='myservice', oauth_token=TOKEN)

printers = equipment.get(lookup={
    'floor.office.city.name.en': 'Moscow',
    'floor.number': 5,
    'type': 'printer',
})

for printer in printers:
    print printer['name']

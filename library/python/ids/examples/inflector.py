# coding: utf-8
from __future__ import unicode_literals
import logging

from ids.registry import registry

logging.basicConfig(level=logging.DEBUG)


inflector = registry.get_repository('inflector', 'inflector', user_agent='myservice')


class Person():
    def __init__(self, first_name, last_name, middle_name='', gender=None):
        self.first_name = first_name
        self.last_name = last_name
        self.middle_name = middle_name
        self.gender = gender


## Шорткаты
#
print inflector.inflect_string('автомобиль', 'именительный')
print inflector.inflect_string('автомобиль', 'дат')
print inflector.inflect_string('автомобиль', 'чем')
print inflector.inflect_person('Петр Петров', 'кому')
print inflector.inflect_person({'first_name': 'Петр', 'last_name': 'Петров'}, 'кому')
print inflector.inflect_person(Person('Петр', 'Петров', gender='m'), 'кому')


res = inflector.get_string_inflections('автомобиль')
print res
print "Им как атрибут: ", res.nominative
print "Род как атрибут: ", res.genitive
print "Вин как атрибут: ",res.accusative
print "Дат как атрибут: ",res.dative
print "Твор как атрибут: ",res.instrumental
print "Предл как атрибут: ",res.prepositional

print "Склонения через getitem", res['кого']
print "Склонения через getitem", res['дательный']

for inflected_person in [
    inflector.get_person_inflections({'first_name': 'Петр', 'last_name': 'Петров'}),
    inflector.get_person_inflections(Person('Петр', 'Петров', gender='m'))
]:
    print "Склонения через getitem: ", inflected_person['кого']
    print "Склонения через getitem: ", inflected_person['дательный']
    print "Формат: ", inflected_person.format('{last_name} {first_name}', 'именительный')

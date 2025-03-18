# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from pprint import pprint
from ids.registry import registry


kostroma_id = 7
country_type = 3


region = registry.get_repository('geobase', 'region', user_agent='example')
print '{id}, {name}, {en_name}, {type}'.format(**region.get_one({'id': kostroma_id}))

for it in region.get_all({'type': country_type})[:5]:
    print '{id}, {name}, {en_name}, {type}'.format(**it)


parents = registry.get_repository('geobase', 'parents', user_agent='example')
pprint(parents.get_all({'id': kostroma_id}))

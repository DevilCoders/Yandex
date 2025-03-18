# coding: utf-8
from __future__ import unicode_literals

from collections import namedtuple

# пока так, можно потом сделать что-то посложнее
Absence = namedtuple('Absence', 'date_from date_to state type id login')

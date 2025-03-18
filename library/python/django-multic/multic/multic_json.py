# coding: utf-8

from simplejson import JSONEncoder
from datetime import datetime, date, time
import re


class CustomJSONEncoder(JSONEncoder):
    def default(self, o):
        if isinstance(o, (datetime, time, date)):
            return re.sub('\.\d{6}$', '', o.isoformat())
        elif isinstance(o, set):
            return list(o)
        else:
            return super(CustomJSONEncoder, self).default(o)

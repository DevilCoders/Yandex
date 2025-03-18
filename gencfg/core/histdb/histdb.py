#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
import copy
import time

from gaux.aux_mongo import get_mongo_collection

from events import TEvent


class Histdb(object):
    def __init__(self, db):
        self.db = db
        self.mongocoll = get_mongo_collection('histdb')

        # get maximum event id and set nextid
        # self.nextid = self.mongocoll.find( { }, { 'event_id' : 1 } ).sort('event_id',  -1)[0]['event_id'] + 1
        self.nextid = None

        self.last_check_time = 0
        self.last_check_exception = None

    def load_events(self, event_type=None, event_name=None, event_object_id=None):
        d = {}
        if event_type is not None:
            d['event_type'] = event_type
        if event_name is not None:
            d['event_name'] = event_name
        if event_object_id is not None:
            d['event_object_id'] = event_object_id

        for elem in self.mongocoll.find(d):
            if elem['event_id'] not in TEvent.EVENTS_BY_ID:
                TEvent(elem['event_type'], elem['event_name'], elem['event_object_id'], new_event=False,
                       event_params=elem['event_params'], event_date=elem['event_date'], event_id=elem['event_id'])

        self.events_loaded = True

    def get_next_id(self):
        if self.nextid is None:
            self.nextid = self.mongocoll.find({}, {'event_id': 1}).sort('event_id', -1)[0]['event_id'] + 1
        self.nextid += 1
        return self.nextid - 1

    def get_events(self, event_type=None, event_name=None, event_object_id=None, flt=lambda x: True, load=True):
        if load:
            self.load_events(event_type, event_name, event_object_id)

        extra_flt = lambda x: (event_type is None or x.event_type == event_type) and (
            event_name is None or x.event_name == event_name)

        if event_object_id is None:
            return sorted(filter(lambda x: extra_flt(x) and flt(x), TEvent.EVENTS_BY_ID.values()),
                          cmp=lambda x, y: cmp(x.event_id, y.event_id))
        else:
            return sorted(filter(lambda x: extra_flt(x) and flt(x), TEvent.EVENTS_BY_OBJECT_ID[event_object_id]),
                          cmp=lambda x, y: cmp(x.event_id, y.event_id))

    # for every event get previous event of same 'type' and same 'name'. Show only properties, which were changed
    def get_smart_params(self, events):
        d_by_type_name = defaultdict(list)

        result = []
        for event in events:
            edict = {}

            type_name = (event.event_type, event.event_name)
            if len(d_by_type_name[type_name]) > 0:
                for k, v in event.event_params.iteritems():
                    if k in d_by_type_name[type_name][-1].event_params and d_by_type_name[type_name][-1].event_params[
                        k] == v:
                        continue
                    edict[k] = v
            else:
                edict = copy.deepcopy(event.event_params)
            d_by_type_name[type_name].append(event)

            estr = ', '.join(map(lambda x: '%s = %s' % (x, edict[x]), sorted(edict.keys())))
            result.append(estr)

        return result

    def has_events(self, event_type=None, event_name=None, event_object_id=None, flt=lambda x: True, load=True):
        return len(self.get_events(event_type, event_name, event_object_id, flt, load=load)) > 0

    def show_events(self, event_type=None, event_name=None, event_object_id=None, flt=lambda x: True, load=True):
        for event in self.get_events(event_type, event_name, event_object_id, flt, load=load):
            print event

    def update(self, smart=False):
        if smart:
            # cannot be smart updated
            return

        for event in filter(lambda x: x.new_event, TEvent.EVENTS_BY_ID.values()):
            self.mongocoll.insert({
                'event_id': event.event_id,
                'event_type': event.event_type,
                'event_name': event.event_name,
                'event_object_id': event.event_object.object_id,
                'event_params': event.event_params,
                'event_date': event.event_date
            })

    def fast_check(self, timeout):
        del timeout
        current_time = time.time()
        if current_time < self.last_check_time:
            self.last_check_time = 0
            self.last_check_exception = None

        # will do real check not more than once in a minute
        need_to_check = (current_time - self.last_check_time >= 60)

        if not need_to_check:
            if self.last_check_exception:
                raise copy.copy(self.last_check_exception)
            return

        self.last_check_time = current_time
        try:
            self.__fast_check()
            self.last_check_exception = None
        except Exception as err:
            self.last_check_exception = copy.copy(err)
            raise err

    def __fast_check(self):
        get_mongo_collection('instanceusage').find_one()

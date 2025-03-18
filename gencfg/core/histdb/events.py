#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
from collections import defaultdict


class EEventTypes(object):
    HOST = 'HOST'
    GROUP = 'GROUP'
    METAGROUP = 'METAGROUP'
    INTLOOKUP = 'INTLOOKUP'
    TAG = 'TAG'
    ALL = [HOST, GROUP, METAGROUP, INTLOOKUP, TAG]


class ETagEvents(object):
    CREATED = 'CREATED'


class EHostEvents(object):
    ADDED = 'ADDED'
    HWMODIFIED = 'HWMODIFIED'
    GROUPCHANGED = 'GROUPCHANGED'
    REMOVED = 'REMOVED'


class EIntlookupEvents(object):
    ADDED = 'ADDED'


class EGroupEvents(object):
    ADDED = 'ADDED'
    RENAMED = 'RENAMED'
    STATS = 'STATS'  # group stats (added when create tag)
    REMOVED = 'REMOVED'


class EMetaGroupEvents(object):
    ADDED = 'ADDED'
    STATS = 'STATS'
    REMOVED = 'REMOVED'


class TEvent(object):
    __slots__ = ['event_type', 'event_name', 'event_params', 'event_object', 'event_date', 'event_id', 'new_event']

    EVENTS_BY_ID = dict()  # FIXME: ?? Only single event db at a time
    EVENTS_BY_OBJECT_ID = defaultdict(set)

    def __init__(self, event_type, event_name, event_object_id, new_event=True, event_params=None, event_date=None,
                 event_id=None):
        if event_params is None:
            event_params = {}
        self.event_type = event_type
        self.event_name = event_name
        self.event_params = event_params
        self.event_object = IEventObject.get_by_id(event_object_id)
        self.event_object.events.append(self)
        if new_event:
            from core.db import CURDB
            if event_date is None:
                self.event_date = int(time.time())
            else:
                self.event_date = event_date
            self.event_id = CURDB.histdb.get_next_id()
            self.new_event = True
        else:
            self.event_date = event_date
            self.event_id = event_id
            self.new_event = False
        self.add_to_indexes()

    def add_to_indexes(self):
        TEvent.EVENTS_BY_ID[self.event_id] = self
        TEvent.EVENTS_BY_OBJECT_ID[self.event_object.object_id].add(self)

    def remove_from_indexes(self):
        TEvent.EVENTS_BY_ID.pop(self.event_id)
        TEvent.EVENTS_BY_OBJECT_ID[self.event_object.object_id].pop(self)

    def __str__(self):
        formattedt = time.strftime('%Y-%m-%dT%H:%M:%S', time.localtime(self.event_date))
        return '%s %s %s %s %s %s %s' % (self.event_type, self.event_name, self.event_object.object_id,
                                         self.new_event, self.event_params, formattedt, self.event_id)


# tag events

class NewTagEvents(object):
    def __init__(self):
        pass

    @staticmethod
    def new_tag(tagname, event_date=None):
        assert (not IEventObject.has_id(tagname))
        return TEvent(EEventTypes.TAG, ETagEvents.CREATED, tagname, event_date=event_date)


# group events

class GroupEvents(object):
    def __init__(self):
        pass

    @staticmethod
    def group_added(groupname, event_date=None):
        assert (not IEventObject.has_id(groupname))
        return TEvent(EEventTypes.GROUP, EGroupEvents.ADDED, groupname, event_date=event_date)

    @staticmethod
    def group_removed(groupname, event_date=None):
        assert (IEventObject.has_id(groupname))
        return TEvent(EEventTypes.GROUP, EGroupEvents.REMOVED, groupname, event_date=event_date)

    @staticmethod
    def group_stats_at_tag(groupname, tagname, db, event_date=None):
        assert (IEventObject.has_id(groupname)), "Group <%s> does not exists" % groupname
        group = db.groups.get_group(groupname)
        stats = {
            'tag': tagname,
            'power': int(sum(map(lambda x: x.power, group.getHosts()))),
            'instances': len(group.get_instances()),
            'usedinstances': len(group.get_busy_instances()),
            'disk': sum(map(lambda x: x.disk, group.getHosts())),
            'ssd': sum(map(lambda x: x.ssd, group.getHosts())),
            'memory': sum(map(lambda x: x.memory, group.getHosts())),
        }
        return TEvent(EEventTypes.GROUP, EGroupEvents.STATS, groupname, event_params=stats, event_date=event_date)


# host events
class HostEvents(object):
    def __init__(self):
        pass

    HOST_HWFIELDS = ['model', 'disk', 'ssd', 'memory', 'os', 'kernel', 'issue']

    @staticmethod
    def host_added(host, oldhostname=None, event_date=None):  # oldhostname is not none if host was renamed
        event_params = dict()
        if oldhostname is not None:
            event_params['oldhostname'] = oldhostname
        return TEvent(EEventTypes.HOST, EHostEvents.ADDED, host.name, event_params=event_params, event_date=event_date)

    @staticmethod
    def host_removed(hostname, newhostname=None, event_date=None):
        event_params = dict()
        if newhostname is not None:
            event_params['newhostname'] = newhostname
        return TEvent(EEventTypes.HOST, EHostEvents.REMOVED, hostname, event_params=event_params, event_date=event_date)

    @staticmethod
    def host_hwfields_modified(host, event_date=None):
        event_params = dict(map(lambda x: (x, getattr(host, x, None)), HostEvents.HOST_HWFIELDS))
        return TEvent(EEventTypes.HOST, EHostEvents.HWMODIFIED, host.name, event_params=event_params,
                      event_date=event_date)

    @staticmethod
    def host_group_changed(host, groupname, tagname, event_date=None):
        event_params = {'groupname': groupname, 'tagname': tagname}
        return TEvent(EEventTypes.HOST, EHostEvents.GROUPCHANGED, host.name, event_params=event_params,
                      event_date=event_date)


# metagroup events
class MetaGroupEvents(object):
    def __init__(self):
        pass

    @staticmethod
    def metagroup_added(descr, filters, event_date=None):
        event_params = {'filters': filters}
        return TEvent(EEventTypes.METAGROUP, EMetaGroupEvents.ADDED, descr, event_params=event_params,
                      event_date=event_date)

    @staticmethod
    def metagroup_stats_at_tag(tuplestats, tagname, event_date=None):
        descr, total_power, total_memory, total_instances, total_hosts = tuplestats
        stats = {'tag': tagname,
                 'power': total_power,
                 'memory': total_memory,
                 'instances': total_instances,
                 'hosts': total_hosts,
                 }
        return TEvent(EEventTypes.METAGROUP, EMetaGroupEvents.STATS, descr, event_params=stats, event_date=event_date)

    @staticmethod
    def metagroup_removed(descr, event_date=None):
        assert (IEventObject.has_id(descr))
        return TEvent(EEventTypes.METAGROUP, EMetaGroupEvents.REMOVED, descr, event_date=event_date)


# event objects (every event belongs to one ojbect, like host, group, intlookup ...)

class IEventObject(object):
    OBJECTS = dict()

    def __init__(self, object_id):
        assert (object_id not in IEventObject.OBJECTS)
        self.object_id = object_id
        self.events = []
        IEventObject.OBJECTS[object_id] = self

    @staticmethod
    def has_id(object_id):
        return object_id in IEventObject.OBJECTS

    @staticmethod
    def get_by_id(object_id):
        if object_id not in IEventObject.OBJECTS:
            IEventObject(object_id)
        return IEventObject.OBJECTS[object_id]


# Tag objects

class THostObject(IEventObject):
    def __init__(self, object_id):
        super(THostObject, self).__init__(object_id)


class TGroupObject(IEventObject):
    def __init__(self, object_id):
        super(TGroupObject, self).__init__(object_id)


class TIntlookupObject(IEventObject):
    def __init__(self, object_id):
        super(TIntlookupObject, self).__init__(object_id)

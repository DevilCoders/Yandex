import tornado
from tornado import gen
from tornado.locks import Event
import datetime
import json



import const
import logdef
import defaults



class UnicornTool(object):
    '''
    A set of methods to deal with unicorn hierarchy
    '''

    def __init__(self):
        pass

    @gen.coroutine
    def UNodeWatcher(self,
                     Unicorn,
                     UNodePath,
                     StoreInObj,
                     EventWaitFor=None,
                     EventSetOnUpdate=None,
                     UTimeOut=const.UNICORN_TIMEOUT,
                     RoutineName='unicorn-watcher'):
        log = logdef.ContextAdapter(logdef.logging.getLogger('orchestrator'), {'routine': RoutineName})
        if EventWaitFor is not None:
            log.debug('Started, but waiting for lock clearance.')
            yield EventWaitFor.wait()
        else:
            log.debug('Started without lock waiting.')
        log.info('Making subscription to %s', UNodePath)
        try:
            u = yield Unicorn.subscribe(UNodePath)
            result = yield u.rx.get(UTimeOut)
        except:
            log.critical('Trace:\n\n', exc_info=True)
            raise EOFError('Subscription to service Unicorn failed!')
        else:
            log.info('Received: %s', UNodePath)
            try:
                yield StoreInObj.update(value=result)
                if EventSetOnUpdate is not None:
                    EventSetOnUpdate.set()
            except:
                log.critical('Trace:\n\n', exc_info=True)
                raise RuntimeError('Failed to store data in StoreInObj!')
        while True:
            try:
                result = yield u.rx.get()
            except:
                log.critical('Trace:\n\n', exc_info=True)
                raise EOFError('Communication with service Unicorn failed!')
            else:
                log.info('Detected update: %s', UNodePath)
                yield StoreInObj.update(value=result)
                if EventSetOnUpdate is not None:
                    EventSetOnUpdate.set()
                log.debug('Starting to wait another event...')
        yield u.tx.close()

    @gen.coroutine
    def UNodeStore(self, Unicorn, UNodePath, Value, UTimeOut=const.UNICORN_TIMEOUT):
        try:
            uget = yield Unicorn.get(UNodePath)
            result = yield uget.rx.get(UTimeOut)
        except:
            raise EOFError('Communication with service unicorn failed!')
        else:
            if result[0] is None:
                try:
                    ucreate = yield Unicorn.create(UNodePath, Value)
                    yield ucreate.rx.get(UTimeOut)
                except:
                    raise EOFError('Failed store value to unicorn node!')
                finally:
                    yield ucreate.tx.close()
            else:
                try:
                    uput = yield Unicorn.put(UNodePath, Value, result[1])
                    yield uput.rx.get(UTimeOut)
                except Exception as e:
                    raise EOFError('Failed store value to unicorn node!\nTrace:\n%s', e)
                finally:
                    yield uput.tx.close()
        finally:
            yield uget.tx.close()

    @gen.coroutine
    def UTreeWatcher(self, Unicorn, UPath, UTimeOut=const.UNICORN_TIMEOUT):
        raise NotImplementedError

    @gen.coroutine
    def UNodeRemove(self, Unicorn, UNodePath, UTimeOut=const.UNICORN_TIMEOUT):
        raise NotImplementedError


class State(object):

    def __init__(self, path):
        self.state = {}
        self.state.setdefault(const.UNICORN_COMMON_HOST_STATE_JSON_KEY_COMPOSEDTIMESTAMPUTC,
                              datetime.datetime.utcnow().isoformat())
        self.version = None
        self.path = path
        self.free = Event()
        self.free.set()

        self.log = logdef.ContextAdapter(logdef.logging.getLogger('orchestrator'), {'routine': 'state-object'})

    def __repr__(self):
        res = json.dumps(self.state, sort_keys=True, separators=(',', ':'))
        return res

    @gen.coroutine
    def caststr(self):
        yield self.free.wait()
        res = json.dumps(self.state, sort_keys=True, separators=(',', ':'))
        raise gen.Return(res)

    @gen.coroutine
    def show_pretty(self):
        yield self.free.wait()
        res = json.dumps(self.state, sort_keys=True, indent=4, separators=(',', ': '))
        raise gen.Return(res)

    @gen.coroutine
    def get(self):
        yield self.free.wait()
        raise gen.Return(self.state)

    @gen.coroutine
    def update(self, value):
        yield self.free.wait()
        try:
            self.free.clear()
            if isinstance(value, dict):
                self.state.update(value)
            elif isinstance(value, list):
                try:
                    jdict = json.loads(value[0])
                except:
                    raise ValueError('Can not parse state')
                else:
                    self.state.update(jdict)
                    self.version = value[1]
                    self.state.update({const.UNICORN_COMMON_HOST_STATE_JSON_KEY_COMPOSEDTIMESTAMPUTC:
                                       datetime.datetime.utcnow().isoformat()})
            else:
                raise ValueError
        except:
            raise ValueError
        finally:
            self.free.set()


class InfraItem(object):

    def __init__(self, Name):
        self.Name = Name


class Application(InfraItem):

    def __init__(self, Name):
        super(Application, self).__init__(Name)
        self.Version = None
        self.Workers = None
        self.RawName = None
        self.ProfileName = None


class DataCenter(InfraItem):

    def __init__(self, Name):
        super(DataCenter, self).__init__(Name)
        self.Agents = []


class Cluster(InfraItem):

    def __init__(self, Name):
        super(Cluster, self).__init__(Name)
        self.DataCenters = []


class Environment(InfraItem):

    def __init__(self, Name):
        super(Environment, self).__init__(Name)
        self.Clusters = []


class Applications(object):
    def __init__(self):
        self.AppList = []

    def __str__(self):
        return json.dumps(dict((a.RawName, a.ProfileName) for a in self.AppList), sort_keys=True, indent=4, separators=(',', ': '))

    def Find(self, Value=None, AttrList=None):
        ''' Returns list of applications, which value contains in attrlist.
        Returns list of all applications if attrlist is None '''
        ret = []
        if AttrList is not None:
            for o in self.AppList:
                for a in AttrList:
                    if hasattr(o, a):
                        if getattr(o, a) == Value:
                            ret.append(o)
        else:
            ret = self.AppList
        return ret

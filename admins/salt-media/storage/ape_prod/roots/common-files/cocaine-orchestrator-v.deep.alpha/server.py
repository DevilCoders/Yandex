#!/usr/bin/env python

import tornado
from tornado import gen
# to prevent mutual import we need to import obviously
from tornado.locks import Event
# to prevent mutual import we need to import obviously
from tornado import httpclient

import json
import signal
import argparse
import re
import copy
import datetime
from math import ceil
import random
import collections

from cocaine.services import Service

import const
import defaults
import common
import logdef
from logdef import *


class State(common.State):
    ''' Agent's state representation on server side.
    Has reference to agent, which state this object represents itself.'''

    def __init__(self, path):
        super(State, self).__init__(path)
        self.agent = None

    def remove_app(self, app):
        ''' Method removes given application from state tree with it's siblings recursively. '''
        if app is not None:
            try:
                self.state[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP][app].clear()
                self.state[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP].pop(app)
            except KeyError:
                raise

    def add_app(self, app, profile):
        ''' Method adds given application and profile to state tree. '''
        if app is not None:
            try:
                self.state[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP].update({app: {const.UNICORN_COMMON_HOST_STATE_JSON_KEY_PROFILE_TREE: {const.UNICORN_COMMON_HOST_STATE_JSON_KEY_PROFILE_NAME: profile}}})
            except KeyError:
                raise


class Configuration(object):
    ''' cocaine orchestrator server configuration, dict '''

    def __init__(self):
        self.conf = None
        self.version = None
        # event.set() on every conf change
        self.event = Event()

    def __repr__(self):
        ''' returns JSON representation of configuration with pretty look. '''
        conf = copy.deepcopy(self.conf)
        conf.update({'version': self.version})
        return json.dumps(conf, sort_keys=True, indent=4, separators=(',', ': '))

    def get(self):
        ''' method returns stored configuration as dict '''
        return self.conf

    def clear(self):
        ''' method sets configuration value to None '''
        self.conf = None

    @gen.coroutine
    def update(self, value):
        ''' method expects tuple with configuration data
        in json representation [0] and it's version [1],
        checking syntax and stores new conf as dict on success
        and calls event.set()
        look further get_event() method '''
        if value is not None:
            try:
                jdict = json.loads(value[0])
            except:
                msg = 'Configuration syntax check failed.'
                if self.conf is not None:
                    msg = msg + ' Keep previous version %d.' % self.version
                raise ValueError('%s' % msg)
            else:
                if self.conf != jdict:
                    self.conf = jdict
                    self.version = value[1]
                    self.event.set()
                    self.event.clear()
#                    yield gen.Return(0)
        else:
            raise ValueError('Configuration is empty!')

    def get_event(self):
        ''' method returns tornado.event used for
        triggering on configuration update '''
        return self.event


class Agent(object):
    '''Representation of cocaine orchestrator agent on server side'''

    def __init__(self, fqdn):
        self.last_seen = None
        self.fqdn = fqdn
        self.datacenter = None
        self.state_from_ref = None
        self.state_to_ref = None
        self.__watchers = []
        self.state_event = Event()
        self.state_event.set()
        self.log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'agent-object'})

    def __repr__(self):
        return self.fqdn

    @gen.coroutine
    def get_watchers(self):
        ''' returns watchers list associated with agent '''
        yield self.state_event.wait()
        raise gen.Return(self.__watchers)

    @gen.coroutine
    def add_watcher(self, value):
        ''' adds named watcher to watchers list associated with agent '''
        try:
            yield self.state_event.wait()
            self.state_event.clear()
            self.__watchers.append(value)
        except:
            raise ValueError
        finally:
            self.state_event.set()

    @gen.coroutine
    def del_watcher(self, value):
        ''' deletes named watcher from watchers list associated with agent '''
        if value is not None:
            try:
                yield self.state_event.wait()
                self.state_event.clear()
                try:
                    while True:
                        self.__watchers.remove(value)
                except:
                    pass
            finally:
                self.state_event.set()

    @gen.coroutine
    def state_diff(self):
        ''' returns list of applications which differs "FROM"-state to "TO"-state associated with agent. '''
        if self.state_from_ref is not None and self.state_to_ref is not None:
            sf = yield self.state_from_ref.get()
            sfapps = sf[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP].keys()
            st = yield self.state_to_ref.get()
            stapps = st[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP].keys()
            res = list(set(sfapps) - set(stapps))
            raise gen.Return(res)
        else:
            raise ValueError('At least one of the states "from" or "to" is None.')

    @gen.coroutine
    def need_update(self):
        ''' returns True if applications lists differs between "FROM"- and "TO"-states associated with agent '''
        diff = yield self.state_diff()
        raise gen.Return(len(diff) > 0)

    @gen.coroutine
    def statecp_from_to(self):
        ''' makes independent copy of "FROM"-state to "TO"-state. '''
        sf = yield self.state_from_ref.get()
        nodepath = '%s/%s%s' % (const.UNICORN_AGENT_HOST_TO_ROOT, self.fqdn, const.UNICORN_SUFFIX_STATE)
        self.state_to_ref = State(path=nodepath)
        st = copy.deepcopy(sf)
        yield self.state_to_ref.update(st)

    def is_active(self):
        ''' return True if agent sent heartbeat recently and False if not. '''
        if self.last_seen is not None:
            diff = datetime.datetime.utcnow() - self.last_seen
            return diff.seconds < const.UNICORN_HEARTBEAT_LOST_THRESHOLD

    @gen.coroutine
    def TreatState(self, Value, StateToDCFlag):
        ''' Parse JSON and store it in "FROM"-state associated with agent '''
        if Value:
            try:
                jdict = json.loads(Value[0])
            except:
                raise ValueError('Can not parse state')
            else:
                yield self.state_from_ref.update(jdict)
                self.state_from_ref.version = Value[1]
                if StateToDCFlag:
                    if jdict[const.UNICORN_AGENT_HOST_STATE_JSON_KEY_DC] != defaults.AGENT_DATACENTER:
                        self.datacenter = jdict[const.UNICORN_AGENT_HOST_STATE_JSON_KEY_DC]
                else:
                    raise UserWarning('get from depth_check/backends_list is not implemented yet')


class Agents(object):
    ''' Agents collection '''

    def __init__(self):
        self.__agents = []
        self.log = logdef.ContextAdapter(logdef.logging.getLogger('orchestrator'), {'routine': 'agents-aggregator'})

    def __getitem__(self, index):
        return self.__agents[index]

    def __setitem__(self, index, value):
        self.__agents[index] = value

    def __repr__(self):
        ''' return string, which represents collection with pretty look '''
        online = []
        offline = []
        for i in self.__agents:
            a = '%s:%s' % (i.fqdn, i.datacenter)
            if i.is_active():
                online.append(a)
            else:
                offline.append(a)
        res = "online: [%s]\toffline: [%s]" % (', '.join(online), ', '.join(offline))
        return res

    def append(self, value):
        ''' adds given agent to the collection '''
        self.__agents.append(value)

    def active_list(self):
        ''' returns list of agents, which sent heartbeat recently '''
        return [a for a in self.__agents if a.is_active()]

    def dclist(self):
        ''' returns list of unique datacenters names, where active agents are presents '''
        dcs = set(a.datacenter for a in self.active_list())
        res = [i for i in dcs if i is not None and i != defaults.AGENT_DATACENTER]
        res.sort()
        return res

    def find(self, value=None, attrlist=None):
        ''' Returns list of agents, which value contains in attrlist.
        Returns list of all agents if attrlist is None '''
        ret = []
        if attrlist is not None:
            for o in self.__agents:
                for a in attrlist:
                    if hasattr(o, a):
                        if getattr(o, a) == value:
                            ret.append(o)
        else:
            ret = self.__agents
        return ret


class PipeLineClient(object):

    def __init__(self, Host=defaults.PIPELINE_HOST, Port=defaults.PIPELINE_PORT):
        self.Host = Host
        self.Port = Port
        self.Client = httpclient.AsyncHTTPClient()

    @gen.coroutine
    def Get(self, Path):
        url = "https://%s/%s" % (self.Host, Path)
        try:
            response = yield self.Client.fetch(url, validate_cert=False)
        except:
            raise
        raise gen.Return(response)


class Applications(common.Applications):

    def __init__(self, ClusterName, PipeLineHost=None):
        super(Applications, self).__init__()
        self.PipeLineHost = PipeLineHost
        self.ClusterName = ClusterName
        self.log = logdef.ContextAdapter(logdef.logging.getLogger('orchestrator'), {'routine': 'apps-collection'})

    @gen.coroutine
    def GenFromRunList(self):
        try:
            PL = PipeLineClient(Host=self.PipeLineHost)
            Path = 'cluster2runlist/%s' % self.ClusterName
            RunList = yield PL.Get(Path)
            if RunList.code == 200:
                try:
                    Apps = json.loads(RunList.body)
                except:
                    raise
                self.AppList = []
                for AppName in Apps.keys():
                    app = common.Application(AppName)
                    app.RawName = AppName
                    app.ProfileName = Apps[AppName]
                    self.AppList.append(app)
            else:
                self.log.warning('Pipeline runlist fetch status code is NOT 200 OK!')
        except:
            self.log.critical('Trace:\n\n', exc_info=True)


class LeadStates(object):
    ''' Provides methods for accounting datacenter/backend/worker
    It doesn't data storage. '''

    def __init__(self):
        pass

    @gen.coroutine
    def CreateStates(self, AgentsCollection, AppCollection, Config):
        ''' Takes agents list + config with list of predefined application names
        and it's workers count per DC. Calculates amount of backends needed to
        applications run on and produces modified states for agents
        with list of applications should work on certain cocaine-backend.
        Each agent object must have two states:
        state_from_ref -- state received from agent
        state_to_ref -- changes made by this method will be stored here
        '''
        log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'state-composer'})
        # collect here list of applications, which don't mentioned config
        apps_not_managed = []
        # agents contains all agents from unicorn, with active_list() we taking only which sent state recently
        active = AgentsCollection.active_list()
        for agent in active:
            yield agent.statecp_from_to()
        # get unique datacenters names from active agents
        datacenters = AgentsCollection.dclist()
        log.debug('Datacenters list: %s', datacenters)
        log.debug('Accounting started.')
        try:
            # acc -- accounting data storage
            acc = collections.defaultdict()
            # acc[dcname]
            for d in datacenters:
                acc[d] = None
            for dc in acc.keys():
                # acc[dcname]['apps'] -- dict with details (values) about appname (key) in certain DC
                # acc[dcname]['agents'] -- agents list in certain DC
                acc[dc] = {'apps': {}, 'agents': []}
            # processing each active agent
            for agent in active:
                # add agent to the list of agents in appropriate datacenter
                acc[agent.datacenter]['agents'].append(agent)
                # getting list of applications running on current agent
                apps = agent.state_from_ref.state[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP]
                for appname in apps.keys():
                    try:
                        # this key available only for running app, there is no such key for broken app or anything else
                        workers = apps[appname]['pool']['capacity']
                        # creating key associated with app in current DC
                        acc[agent.datacenter]['apps'].setdefault(appname, {})
                        # workers -- count of workers currently available in certain DC
                        acc[agent.datacenter]['apps'][appname].setdefault('workers', 0)
                        # agents -- list of agents app running on in certain DC
                        acc[agent.datacenter]['apps'][appname].setdefault('agents', [])
                        acc[agent.datacenter]['apps'][appname]['workers'] += workers
                        acc[agent.datacenter]['apps'][appname]['agents'].append(agent)
                    except KeyError:
                        log.debug('App %s on agent %s has no key pool:capacity. Is it broken? Skipping.', appname, agent)
            # here we have in acc:
            # acc -- dict of available datacenters
            # acc[dcname]['agents'] -- list of all agents in the named datacenter
            # acc[dcname]['apps'] -- dict of available applications in the named datacenter
            # acc[dcname]['apps'][appname]['workers'] -- count of workers for named application
            # acc[dcname]['apps'][appname]['agents'] -- list of agents running named application
        except:
            log.debug('Trace:\n', exc_info=True)
        log.debug('Accounting ended.')
        # counter for atomic modifications we'll made with agent's state
        change_counter = 0
        try:
            apps = Config.conf[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP]
        except KeyError:
            log.warning('Config has no required key "%s" with list of controlled applications!', const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP)
        else:
            # list of applications running on backends, but not mentioned in server config
            apps_not_managed = []
            try:
                for dc in acc.keys():
                    # store apps which are not in config
                    apps_not_managed.extend(list(set(acc[dc]['apps'].keys()) - set(apps)))
                    log.debug('Iterating over datacenter: %s', dc)
                    try:
                        for app in apps:
                            if app in acc[dc]['apps'].keys():
                                log.debug('Processing app=%s.', app)
                                # using formulae: min(dccapacity, max(3, ceil(workers / poollimit)))
                                # https://st.yandex-team.ru/COCAINE-1926#1470726232000
                                # poollimit = total possible workers count / backends -- it just the same as pool::capacity
                                poollimit = float(acc[dc]['apps'][app]['workers']) / len(acc[dc]['apps'][app]['agents'])
                                # workers -- value from server config, desired workers count per DC
                                workers = float(Config.conf[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP][app][const.UNICORN_SERVER_CONF_FILE_KEY_WORKERS])
                                # minbackends -- hardcoded bottom bound to prevent app stop everywhere
                                minbackends = const.CALC_MIN_BACKENDS_PER_APP
                                # dccapacity -- total backend count, don't matter where certain app running on
                                dccapacity = len(acc[dc]['agents'])
                                # desired backends count appropriate current server settings for certain app
                                backends = max(minbackends, int(ceil(workers / poollimit)))
                                # shrink backends count if desired value exceeds value of available backends
                                decision = min(dccapacity, backends)
                                log.debug('min(dccapacity, max(3, ceil(workers / poollimit))) = min(%d, max(3, ceil(%d / %d)))', dccapacity, workers, poollimit)
                                if minbackends > dccapacity:
                                    log.warning('Hardcoded minimal backends count per DC is %d, but only %d is active!', minbackends, dccapacity)
                                if backends > dccapacity > minbackends:
                                    log.warning('DC=%s Application %s reqested %d backends, but only %d available, they all will run this app.', dc, app, backends, dccapacity)
                                log.debug('DC=%s Backends_in_DC=%d App=%s Workers_defined_at_conf=%d Backends_running_app_now:%d Backends_needed=%d', dc, dccapacity, app, workers, len(acc[dc]['apps'][app]['agents']), decision)
                                log.debug('App %s running on agents: %s', app, acc[dc]['apps'][app]['agents'])
                                # actual -- count of backends running app right now
                                actual = len(acc[dc]['apps'][app]['agents'])
                                if actual > decision:
                                    # current backends count is great than desired
                                    log.info('DC=%s App %s running on large number of backends than required %d, trying to stop redundant.', dc, app, decision)
                                    # amount of backends, where we'll try to stop app
                                    diff = actual - decision
                                    # exclude -- list of backends, where app already requested to stop
                                    exclude = []
                                    # iterating over backends with running app
                                    for agent in acc[dc]['apps'][app]['agents']:
                                        try:
                                            # getting difference between agent's self-reported state and state modified by server, if any
                                            appdiff = yield agent.state_diff()
                                        except ValueError as e:
                                            log.warning('BUG!\n%s\nTrace:\n', e, exc_info=True)
                                        else:
                                            # checking if current backend has no pending request on stop for current app
                                            if app in appdiff:
                                                log.warning('Agent %s still processing app=%s. Race condition? BUG?', agent.fqdn, app)
                                                # adding backend to exclude list
                                                exclude.append(agent)
                                    # delta is amount of backends has been requested to stop app already ; usually it should be ZERO
                                    delta = len(exclude)
                                    while delta < diff:
                                        # assume we modifying agent's state one time more
                                        change_counter += 1
                                        # getting random agent from residual list of agents (except excluded)
                                        victim = random.choice(list(set(acc[dc]['apps'][app]['agents']) - set(exclude)))
                                        # appending this agent to excludes, cause it musn't appear in next iteration
                                        exclude.append(victim)
                                        # removing app from generated state of agent
                                        victim.state_to_ref.remove_app(app)
                                        # encounting current list of apps requested to stop on this agent
                                        appdiff = yield victim.state_diff()
                                        log.debug('Applist diff on agent %s is: %s', victim, appdiff)
                                        # increasing cycle counter
                                        delta += 1
                                elif actual < decision:
                                    # current backends count is less than desired
                                    log.info('DC=%s App %s running on less number of backends than required %d, trying to start.', dc, app, decision)
                                    search = AppCollection.Find(Value=app, AttrList=['RawName'])
                                    if len(search) == 1:
                                        AppObj = search.pop()
                                        log.debug('App: %s\tProfile: %s', AppObj.RawName, AppObj.ProfileName)
                                        # amount of backends, where we'll try to start app
                                        diff = decision - actual
                                        # cycle counter
                                        delta = 0
                                        # exclude -- list of backends, where app already requested to start
                                        exclude = []
                                        while delta < diff:
                                            # assume we modifying agent's state one time more
                                            change_counter += 1
                                            agents_with_app = exclude
                                            agents_with_app.extend(acc[dc]['apps'][app]['agents'])
                                            # getting random agent from residual list of agents (except excluded)
                                            victim = random.choice(list(set(acc[dc]['agents']) - set(agents_with_app)))
                                            # appending this agent to excludes, cause it musn't appear in next iteration
                                            exclude.append(victim)
                                            # adding app to generated state
                                            victim.state_to_ref.add_app(app=app, profile=AppObj.ProfileName)
                                            # encounting current list of apps requested to stop on this agent
                                            appdiff = yield victim.state_diff()
                                            log.debug('Applist diff on agent %s is: %s', victim, appdiff)
                                            # increasing cycle counter
                                            delta += 1
                                    elif len(search) > 1:
                                        log.warning('More than one (%d) application has name %s. BUG! Skipping app.', len(search), app)
                                    else:
                                        log.warning('Application %s not found in runlist! Is it orphaned?', app)
#                                    log.warning('DC=%s App %s running on fewer backends than required %d, but application start is not implemented yet cause server does not know app profile name.', dc, app, decision)
                                else:
                                    log.debug('DC=%s App %s is in desired state, no start/stop needed.', dc, app)
                            else:
                                log.info('DC=%s Application %s not found.', dc, app)
                    except:
                        log.debug('Trace:\n', exc_info=True)
                # here we have duplicated values in the list (one subset for each datacenter)
                # removing dups, storing unique app names
                apps_not_managed = list(set(apps_not_managed))
                # soring list for pretty look in debug message
                apps_not_managed.sort()
                log.debug('List of not managed applications from all DCs: %s', apps_not_managed)
            except KeyError:
                log.warning('Error while processing dc: ', dc)
        # reporting changes made for each affected agent
        for a in active:
            diff = yield a.state_diff()
            if len(diff) > 0:
                log.info('Agent %s application list difference is: %s', a.fqdn, diff)
        raise gen.Return(change_counter)

    @gen.coroutine
    def SendStates(self, Unicorn, AgentsCollection, OnlyIfChanged=False, UnicornTimeOut=const.UNICORN_TIMEOUT):
        ''' Method send states to corresponding agents via unicorn.
        If OnlyIfChanged is True, then it will skip agents with no modified states,
        but with False (default) it will send states to all. '''
        log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'state-sender'})
        for agent in AgentsCollection.active_list():
            if agent.state_to_ref.path is not None:
                if OnlyIfChanged:
                    # checking if modifications was made
                    need_update = yield agent.need_update()
                else:
                    need_update = True
                if need_update:
                    # getting current version of unicorn node
                    try:
                        u = yield Unicorn.get(agent.state_to_ref.path)
                        result = yield u.rx.get(timeout=const.UNICORN_TIMEOUT)
                    except:
                        exists = False
                    else:
                        exists = result[0] is not None
                    finally:
                        yield u.tx.close()
                    if exists:
                        # node already exists, we'll update it using version number
                        try:
                            state_json = yield agent.state_to_ref.caststr()
                            # converting agent's state to json
                            # overwriting unicorn node using json and node version obtained above
                            u = yield Unicorn.put(agent.state_to_ref.path, state_json, result[1])
                            yield u.rx.get(UnicornTimeOut)
                        except:
                            log.debug('Trace:\n', exc_info=True)
                        else:
                            log.info('Sent state to agent %s, app count: %d', agent.fqdn, len(agent.state_to_ref.state[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP].keys()))
                        finally:
                            yield u.tx.close()
                    else:
                        # node doesn't exist, it should be created
                        # converting agent's state to json
                        state_json = yield agent.state_to_ref.caststr()
                        try:
                            # creating new unicorn node using json
                            u = yield Unicorn.create(agent.state_to_ref.path, state_json)
                            result = yield u.rx.get(UnicornTimeOut)
                        except:
                            log.critical("Service unicorn failed.", exc_info=True)
                            raise EOFError('Failed to create state in unicorn!')
                        else:
                            log.info('State created: %s', agent.state_to_ref.path)
                        finally:
                            yield u.tx.close()


class Server(object):
    ''' Orchestrator server main class.
    You should create one istance of it for each environment dev/testing/production.
    Class contains attributes for synchronization, configuration, unicorn client,
    agents list and methods for reign them all.
    All communication between server and agents based on unicorn service of cocaine.
    It expects that the unicorn service has it's own independed tree of nodes
    for each environment, therefore we need a separate instance for each of them.'''
    def __init__(self,
                 ClusterNames,
                 PipeLineHost=defaults.PIPELINE_HOST,
                 Unicorn=None,
                 UnicornServerLock=const.UNICORN_SERVER_LOCK,
                 UnicornServerConfFile=const.UNICORN_SERVER_CONF_FILE,
                 UnicornFromAgentsTree=const.UNICORN_AGENT_HOST_FROM_ROOT,
                 UnicornToAgentsTree=const.UNICORN_AGENT_HOST_TO_ROOT,
                 UnicornAgentStateFileSuffix=const.UNICORN_SUFFIX_STATE,
                 StateToDCFlag=False):
        if Unicorn is None:
            Unicorn = Service('unicorn', endpoints=[[defaults.LOCATOR_HOST, defaults.LOCATOR_PORT]], timeout=5)
        self.ClusterNames = ClusterNames
        self.PipeLineHost = PipeLineHost
        # here we store a fresh version on configuration received from unicorn
        # assume we apply it only after completion of whole main cycle
        self.PendingConfig = Configuration()
        # service unicorn, it's very, very important
        self.Unicorn = Unicorn
        # path to server's lock in unicorn, thereby only one instance of server should be active in any time
        self.UnicornServerLock = UnicornServerLock
        # path to server's conffile in unicorn
        self.UnicornServerConfFile = UnicornServerConfFile
        # path to agents subtree in unicorn, here we'll monitor agent's reports
        self.UnicornFromAgentsTree = UnicornFromAgentsTree
        # distinguish state-files within subtree UnicornFromAgentsTree from files of other types possible
        self.UnicornAgentFromFileSuffix = UnicornAgentStateFileSuffix
        # path to agents subtree in unicorn, here we'll send states to them
        self.UnicornToAgentsTree = UnicornToAgentsTree
        # if True it will switch on use of datacenter info from agent's state
        self.StateToDCFlag = StateToDCFlag

        self.log = logdef.ContextAdapter(logdef.logging.getLogger('orchestrator'), {'routine': 'initialize'})
        # main ioloop, all coroutines are there
        self.io_loop = tornado.ioloop.IOLoop.current()
        # main IdleFlag, we may stop server without consequences if this flag is set
        self.AllowStopAll = Event()
        # we clear this flag only when we do something important, it must be set all time else
        self.AllowStopAll.set()
        # main flag is set only when server lock is acquired, all coroutines waits it on start
        self.AllowStartAll = Event()
        # list of agents (all found in unicorn, active and inactive)
        self.Agents = Agents()
        # common to server and agent methods for unicorn
        self.UnicornTool = common.UnicornTool()

        # acquiring main lock here, waiting it forever
        # _ALL_ coroutines except this, stay blocked at entry point till AllowStartAll.set() by this coroutine
        self.io_loop.add_future(self.OrchestratorLock(Unicorn=self.Unicorn,
                                                      LockName=self.UnicornServerLock,
                                                      EventSetOnSuccess=self.AllowStartAll),
                                lambda x: [self.log.critical("Timeout getting global lock. Exiting."), self.ScheduleStop(None)])
        # getting here initial configuration and changes made in it after start
        # storing all received data in PendingConfig
        self.io_loop.add_future(self.UnicornTool.UNodeWatcher(Unicorn=self.Unicorn,
                                                              UNodePath=self.UnicornServerConfFile,
                                                              StoreInObj=self.PendingConfig,
                                                              EventWaitFor=self.AllowStartAll,
                                                              RoutineName='conf-watcher'),
                                lambda x: [self.log.critical("ConfWatcher() exited. BUG!"), self.ScheduleStop(None)])
        # main coroutine, coordinates workflow, sets/clears flag IdleFlag during important operations
        # reads changes from PendingConf each iteration
        # requires configuration and waits for it after EventWaitFor, but before enter to main loop
        self.io_loop.add_future(self.Server(Unicorn=self.Unicorn,
                                            PendingConf=self.PendingConfig,
                                            EventWaitFor=self.AllowStartAll,
                                            IdleFlag=self.AllowStopAll,
                                            AgentsCollection=self.Agents,
                                            ClusterNames=self.ClusterNames,
                                            PipeLineHost=self.PipeLineHost),
                                lambda x: [self.log.critical("Server() exited. BUG!"), self.ScheduleStop(None)])
        # watching all agent's states/announces here, subscribing in series to certain nodes
        # adding agent item to AgentsCollection for each node
        # adding dedicated watcher coroutine to ioloop for each node
        # if StateToDCFlag set, agent.datacenter will set using state info
        self.io_loop.add_future(self.SubscribeAgentsTree(Unicorn=self.Unicorn,
                                                         Tree=self.UnicornFromAgentsTree,
                                                         EventWaitFor=self.AllowStartAll,
                                                         AgentsCollection=self.Agents,
                                                         StateToDCFlag=self.StateToDCFlag),
                                lambda x: [self.log.critical("SubscribeAgentsTree() exited. BUG!"), self.ScheduleStop(None)])

    # signal handler, scheduling stop only after cleanup (waiting for interation complete in main cycle)
    def SignalHandler(self, signum, frame):
        log = logdef.ContextAdapter(logdef.logging.getLogger('orchestrator'), {'routine': 'signal-handler'})
        if signum in [signal.SIGINT, signal.SIGTERM]:
            log.info('Signal -%d catched. Scheduling exit...', signum)
            try:
                tornado.ioloop.IOLoop.current().add_callback_from_signal(lambda: self.ScheduleStop(self.AllowStopAll))
            except:
                log.critical('Trace:\n', exc_info=True)
        if signum in [signal.SIGHUP]:
            log.warning('Signal -%d catched. Configuration reload is not implemented yet!', signum)
        if signum in [signal.SIGUSR1]:
            log.warning('Signal -%d catched. Report server status via unicorn is not implemented yet!', signum)

    # stops ioloop after event.set() or
    # immediately if event is None
    @gen.coroutine
    def ScheduleStop(self, event):
        log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'shutdown'})
        if event is not None:
            self.io_loop.add_future(event.wait(),
                                    lambda x: [tornado.ioloop.IOLoop.instance().stop(), log.info('Stopped.')])
        else:
            self.io_loop.add_future(tornado.ioloop.IOLoop.instance().stop(),
                                    lambda x: log.info('Exited immediately without cleanup.'))

    # main server lock acquiring, waits it forever if TimeOut is None
    # sets EventSetOnSuccess.set()
    # LockName -- path to file in unicorn
    @gen.coroutine
    def OrchestratorLock(self, Unicorn, LockName, EventSetOnSuccess, TimeOut=None):
        log = logdef.ContextAdapter(logdef.logging.getLogger('orchestrator'), {'routine': 'global-lock'})
        log.debug('Started.')
        log.info('Trying to get lock in unicorn...')
        try:
            glock = yield Unicorn.lock(LockName)
            yield glock.rx.get(timeout=TimeOut)
        except:
            log.debug("Exception:\n", exc_info=True)
            try:
                yield glock.tx.close()
                log.critical("Lock already exists and belongs to another orchestrator instance")
            except:
                log.critical("\nService unicorn failed.\n", exc_info=True)
            finally:
                self.ScheduleStop(None)
        log.info('Lock acquired: %s', LockName)
        EventSetOnSuccess.set()
        while True:
            yield gen.sleep(const.SERVER_SLEEP_SECONDS)

    # main workflow
    # PendingConf apllied only on iteration completion
    @gen.coroutine
    def Server(self, Unicorn, PendingConf, EventWaitFor, IdleFlag, AgentsCollection, ClusterNames, PipeLineHost):
        # creating clean configuration
        CurrentConf = Configuration()
        log = logdef.ContextAdapter(logdef.logging.getLogger('orchestrator'), {'routine': 'server'})
        log.debug('Started. Waiting for global lock.')
        yield EventWaitFor.wait()
        # will not start until configuration received
        if PendingConf.get() is None:
            log.info('Waiting for configuration...')
            try:
                # stay here until conf-watcher provided configuration
                yield PendingConf.get_event().wait()
            except:
                log.critical('Trace:\n\n', exc_info=True)
                self.ScheduleStop(None)
        AppCollection = Applications(ClusterName=ClusterNames[0], PipeLineHost=PipeLineHost)
        yield AppCollection.GenFromRunList()
        log.debug('Applications from runlist:\n\n%s\n', AppCollection)
        # it is very important to NOT start BEFORE receive hearbeats from all possible active agents
        # giving a chance to them send announce in double heartbeat interval
        gap = const.UNICORN_HEARTBEAT_SLEEP_DURATION * 2
        log.info('Waiting %s seconds for sure to catch announces from all active agents.', gap)
        yield gen.sleep(gap)
        log.info('Awakened. Entering main operating cycle.')
        manage = LeadStates()
        while True:
            if CurrentConf.conf != PendingConf.conf:
                # updating CurrentConf with data of PendingConf
                CurrentConf = copy.deepcopy(PendingConf)
                log.info('Configuration loaded, version is: %d', CurrentConf.version)
            else:
                log.debug('Current config version: %s', CurrentConf.version)
            try:
                log.debug('Setting lock.')
                # entering critical section clearing IdleFlag (SIGTERM or SIGINT don't interrupt us)
                IdleFlag.clear()
                log.debug('Known agents: %s', AgentsCollection)
                try:
                    yield AppCollection.GenFromRunList()
                    log.debug('Applications:\n\n%s\n', AppCollection)
                    # call states generator, which makes changes in agents states
                    manage.CreateStates(AgentsCollection=AgentsCollection, Config=CurrentConf, AppCollection=AppCollection)
                    log.debug('LeadStates.Generate() finished.')
                except:
                    log.critical('Trace:\n\n', exc_info=True)
                else:
                    try:
                        log.info('Sending states to agents.')
                        # call states sender, it brings changes to agents via unicorn
                        yield manage.SendStates(Unicorn=Unicorn, AgentsCollection=AgentsCollection, OnlyIfChanged=False)
                    except:
                        log.critical('Trace:\n\n', exc_info=True)
            except:
                log.critical('Trace:\n\n', exc_info=True)
            finally:
                log.debug('Releasing lock.')
                # exiting critical section
                IdleFlag.set()
            # go to sleep while watcher catches announces, reports, etc.
            # we don't need to start next iteration immediately
            yield gen.sleep(const.SERVER_SLEEP_SECONDS)
            log.debug('Slept %s second(s).', const.SERVER_SLEEP_SECONDS)

    @gen.coroutine
    def SubscribeAgentNode(self, Unicorn, Tree, Item, AgentObj, StateToDCFlag):
        log = logdef.ContextAdapter(logdef.logging.getLogger('orchestrator'), {'routine': 'node-watcher'})
        # using this regex to match agent's state-files
        state_re = re.compile('.*%s$' % self.UnicornAgentFromFileSuffix)
        if re.match(state_re, Item):
            is_state = True
        else:
            is_state = False
        nodepath = '%s/%s' % (Tree, Item)
        if is_state:
            # binding agent to appropriate state instance
            AgentObj.state_from_ref = State(path=nodepath)
            # binding state to appropriate agent instance
            AgentObj.state_from_ref.agent_ref = AgentObj
        # this boundary will be used ONLY in debug messages to prevent huge message flood
        # this amount of chars is enough to determine what has been received by node subscription
        # Repeat: it used ONLY in debug messages further and NO somewhere elsewhere, except debug!
        str_cut_bound = 48
        try:
            # making subscription and receive current node content immediately
            log.debug('Making subscription to: %s', nodepath)
            u = yield Unicorn.subscribe(nodepath)
            result = yield u.rx.get(timeout=const.UNICORN_TIMEOUT)
            if is_state:
                try:
                    # storing received data in agent.state_from_ref
                    AgentObj.TreatState(Value=result, StateToDCFlag=StateToDCFlag)
                except UserWarning as e:
                    log.info(e.msg)
        except:
            log.critical('Trace:\n\n', exc_info=True)
        else:
            log.info('Subscribed to: %s', nodepath)
            AgentObj.add_watcher(Item)
            try:
                if log.isEnabledFor(logdef.logging.DEBUG):
                    first = str(result[0])
                    # only first str_cut_bound will appear in debug message instead of 100500
                    if len(first) > str_cut_bound:
                        first = first[:str_cut_bound] + '...<shortened>'
                    log.debug('Node %s event. Data: %s\tVersion: %s', nodepath, first, result[1])
            except:
                log.critical('Trace:\n\n', exc_info=True)
        # here we have node content and unicorn subscription channel still opened
        while True:
            try:
                result = yield u.rx.get()
                # here we have new fresh node data from unicorn received by subscription
            except Exception, e:
                # here is a exception codes in the name of Overall Nothing!
                # there is no include file with exceptions from unicorn-service, ask antmat@
                if e.servicename == 'unicorn' and e.code == -101 and e.category == 16639:
                    log.warning('Exceptions from unicorn still handled by dirty code!')
                else:
                    log.critical('Unhandled exception catched! Watcher is not watcher anymore. Bye!')
                break
            else:
                log.info('Event: %s', nodepath)
                if log.isEnabledFor(logdef.logging.DEBUG):
                    first = str(result[0])
                    # only first str_cut_bound will appear in debug message instead of 100500
                    if len(first) > str_cut_bound:
                        first = first[:str_cut_bound] + '...<shortened>'
                    log.debug('Node %s event. Data: %s\tVersion: %s', nodepath, first, result[1])
                # VERY IMPORTANT: this update is heartbeat
                # agents with outdated last_seen attribute treated as inactive and ignored by server
                AgentObj.last_seen = datetime.datetime.utcnow()
                if is_state:
                    try:
                        # storing received data in agent.state_from_ref
                        yield AgentObj.TreatState(Value=result, StateToDCFlag=StateToDCFlag)
                    except UserWarning as e:
                        log.warning(e.msg)
        try:
            yield u.tx.close()
        except:
            pass
        log.debug('Removing item agent.watchers=%s', Item)
        AgentObj.del_watcher(Item)

    @gen.coroutine
    def SubscribeAgentsTree(self, Unicorn, Tree, EventWaitFor, AgentsCollection, StateToDCFlag):
        log = logdef.ContextAdapter(logdef.logging.getLogger('orchestrator'), {'routine': 'agents-watcher'})
        log.debug('Started. Waiting for global lock.')
        yield EventWaitFor.wait()
        log.debug('Making subscription to %s', Tree)
        try:
            # making subscription to ALL nodes in Tree, but not recursively!
            # we'll receive changes _only_ on change of nodelist in this place.
            # NOT certain node content!
            # to monitor certain node content change we have to make a dedicated watcher
            u = yield Unicorn.children_subscribe(Tree)
            result = yield u.rx.get(timeout=const.UNICORN_TIMEOUT)
        except:
            try:
                yield u.tx.close()
                log.critical('Node "%s" not found in unicorn.', Tree)
            except:
                log.critical('Service unicorn failed!')
            finally:
                self.ScheduleStop(None)
        else:
            # here we have a subscription on change in children _list_
            log.info('Subscribed to: %s', Tree)
            log.debug('Subscription event: %s', result)
        # skip regex need to exclude locks placed in Tree
        # subscription to lock throws exception immediately
        skip_re = re.compile('.*%s$' % const.UNICORN_SUFFIX_LOCK)
        # using this pattern we'll extract agent's FQDN
        fqdn_re_str = '^([\w\.\-]+)(\.\w+)$'
        fqdn_re = re.compile(fqdn_re_str)
        # iterating over all received nodes by children subscription
        for leaf in result[1]:
            nodepath = '%s/%s' % (Tree, leaf)
            # skipping nodes we don't need to watch
            if not re.match(skip_re, leaf):
                # extracting agent's FQDN
                match = re.match(fqdn_re, leaf)
                if match:
                    fqdn = match.group(1)
                    # tying to find appropriate agent in list of already registered
                    search = AgentsCollection.find(fqdn, {'fqdn'})
                    if len(search) == 0:
                        log.info('Registering new agent: %s.', fqdn)
                        # there is no appropriate agent, creating one
                        agent = Agent(fqdn)
                        # appending it to list
                        AgentsCollection.append(agent)
                    elif len(search) == 1:
                        # reusing existent agent
                        log.debug('Agent %s already registered.', fqdn)
                        agent = search[0]
                    else:
                        # wtf, it should not happen, but we musn't keep it in silence
                        log.warning('Found more than 1 objects matching search criteria fqdn=%s, this should not happen. BUG!', fqdn)
                        continue
                    # adding watcher on certain node, it will bring us content updates
                    self.io_loop.add_future(self.SubscribeAgentNode(Unicorn=Unicorn,
                                                                    Tree=Tree,
                                                                    Item=leaf,
                                                                    AgentObj=agent,
                                                                    StateToDCFlag=StateToDCFlag),
                                            lambda x: log.info("Node watcher exited: %s", nodepath))
                else:
                    log.warning('Can not determine FQDN in item "%s" with pattern "%s"! Item skipped.', leaf, fqdn_re_str)
        while True:
            try:
                # keep watching there for list changes
                result = yield u.rx.get()
            except:
                log.critical('Service unicorn failed!', exc_info=True)
                self.ScheduleStop(None)
            else:
                log.info('Event in tree: %s', Tree)
                for leaf in result[1]:
                    nodepath = '%s/%s' % (Tree, leaf)
                    # skipping nodes we don't need to watch
                    if re.match(skip_re, leaf) is None:
                        # extracting agent's FQDN
                        match = re.match(fqdn_re, leaf)
                        if match:
                            fqdn = match.group(1)
                            try:
                                # tying to find appropriate agent in list of already registered
                                search = AgentsCollection.find(fqdn, {'fqdn'})
                            except:
                                log.critical('Trace:\n\n', exc_info=True)
                            if len(search) == 0:
                                log.debug('Agent with FQDN=%s does not exist yet, creating new one, but it will be marked as online only after first heartbeat.', fqdn)
                                # there is no appropriate agent, creating one
                                agent = Agent(fqdn)
                                # appending it to list
                                AgentsCollection.append(agent)
                                try:
                                    # adding watcher on certain node, it will bring us content updates
                                    self.io_loop.add_future(self.SubscribeAgentNode(Unicorn=Unicorn, Tree=Tree, Item=leaf, AgentObj=agent, StateToDCFlag=StateToDCFlag),
                                                            lambda x: log.info("Node watcher exited:\t%s", nodepath))
                                except:
                                    log.warning('Failed to add new watcher for: %s', nodepath)
                            elif len(search) == 1:
                                # checking if appropriate watcher already exists
                                # hint: on every change in list we receive whole list on nodes and most of them has watchers already
                                if logdef.logging.getLogger().getEffectiveLevel() == logdef.logging.DEBUG:
                                    watchers = yield search[0].get_watchers()
                                log.debug('Agent with FQDN=%s already exists with watchers: %s', fqdn, watchers)
                                try:
                                    for a in search:
                                        watchers = yield a.get_watchers()
                                        if leaf not in watchers:
                                            try:
                                                # there is no such watcher yet, creating new one
                                                self.io_loop.add_future(self.SubscribeAgentNode(Unicorn=Unicorn,
                                                                                                Tree=Tree,
                                                                                                Item=leaf,
                                                                                                AgentObj=a,
                                                                                                StateToDCFlag=StateToDCFlag),
                                                                        lambda x: log.info("Node watcher exited:\t%s", nodepath))
                                                log.debug('Added watcher for %s associated with agent %s', leaf, a.fqdn)
                                            except:
                                                log.warning('Failed to add new watcher for: %s', nodepath)
                                        else:
                                            log.debug('Watcher for %s already registered: %s', leaf, a.fqdn)
                                except:
                                    log.critical('Trace:\n\n', exc_info=True)
                            else:
                                log.warning('Found more than 1 objects matching search criteria fqdn=%s, this should not happen. BUG!', fqdn)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--host", type=str, default=defaults.LOCATOR_HOST, help="locator-service host")
    p.add_argument("--port", type=int, default=defaults.LOCATOR_PORT, help="locator-service port")
    # this option is useless right now, but it make sense in the near future without enforce to change startup-scripts
    p.add_argument("--daemonize", required=True, action='store_true', help="run as archestrator service")
    p.add_argument("--debug", action='store_true', help="enable debug mode and print all messages to stderr")
    p.add_argument("--state2dc", action='store_true', help="True: use datacenter info from agent state; False: use datacenter info from unicorn depth_check/backend_list")
    p.add_argument("--logfile", type=str, default=None, help="path to logfile")
    p.add_argument("--pipelinehost", type=str, default=defaults.PIPELINE_HOST, help="pipeline hostname")
    p.add_argument("--clusternames", required=True, metavar='name', type=str, nargs='+', default=None, help="list of clusternames as defined in pipeline::clusters")
    argv = p.parse_args()

    if argv.debug:
        loglevel = logdef.LOGLEVEL_DEBUG
    else:
        loglevel = logdef.LOGLEVEL_DEFAULT
    logdef.logging.basicConfig(format=logdef.LOG_FORMAT, level=loglevel, filename=argv.logfile)
    log = logdef.ContextAdapter(logdef.logging.getLogger('orchestrator'), {'routine': 'main'})

    log.debug('Using debug logging level.')
    log.debug('Using locator: %s:%d', argv.host, argv.port)
    if argv.state2dc is True:
        log.debug('Using datacenter info from agent state.')
    else:
        log.debug('Using datacenter info from unicorn depth_check/backend_list.')

    unicorn = Service('unicorn', endpoints=[[argv.host, argv.port]], timeout=5)

    log.info('Using clusternames: %s', argv.clusternames)
    if len(argv.clusternames) > 1:
        log.critical('Support for clusters count more than 1 is not implemented yet! Sorry :(')
        raise NotImplementedError

    if argv.daemonize is True:
        log.debug('Daemon mode.')
        try:
            server = Server(Unicorn=unicorn,
                            StateToDCFlag=argv.state2dc,
                            ClusterNames=argv.clusternames,
                            PipeLineHost=argv.pipelinehost)
            signal.signal(signal.SIGINT, server.SignalHandler)
            signal.signal(signal.SIGTERM, server.SignalHandler)
            signal.signal(signal.SIGHUP, server.SignalHandler)
            signal.signal(signal.SIGUSR1, server.SignalHandler)
            log.info('Starting orchestrator server.')
            # starting whole world here
            tornado.ioloop.IOLoop.current().start()
            # ioloop shouldn't stop in normal operation
            # it may be interrupted by user SIGTERM, SIGINT
            # or caused by runtime exception
            log.critical('Main IOLoop finished.')
        except:
            log.critical('Something goes wrong.\n\n', exc_info=True)


# entry point
if __name__ == '__main__':
    try:
        main()
    except:
        exit(1)

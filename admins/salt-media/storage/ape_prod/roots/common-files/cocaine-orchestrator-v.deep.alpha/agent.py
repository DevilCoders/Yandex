#!/usr/bin/env python
import tornado
from tornado import gen
from tornado.locks import Event

import signal
import argparse
import datetime

from cocaine.services import Service

from logdef import *
import const
import defaults
import logdef
import common


class State(common.State):
    ''' Agent's state representation on agent side. '''
    def __init__(self, path):
        super(State, self).__init__(path)
        self.state.setdefault(const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP)
        # here we store datacenter name, if it provided in command line / config
        # server will use it to group agents by DC
        self.state.setdefault(const.UNICORN_AGENT_HOST_STATE_JSON_KEY_DC)
        # agent's full qualified domain name will be stored here
        self.state.setdefault(const.UNICORN_AGENT_HOST_STATE_JSON_KEY_FQDN)
        # this flag set on update making possible to process new data by handlers
        self.UpdateTrigger = Event()


class Agent(object):
    ''' Orchestrator agent main class.
    All communication between agent and server based on unicorn service of cocaine.
    It expects that the unicorn service has it's own independed tree of nodes
    for each environment. '''

    def __init__(self,
                 DataCenter,
                 FQDN,
                 DryRun=False,
                 Unicorn=None,
                 Node=None,
                 UnicornAgentLockFile=const.UNICORN_AGENT_HOST_LOCK_FILE,
                 UnicornAgentFromFile=const.UNICORN_AGENT_HOST_FROM_FILE,
                 UnicornAgentToFile=const.UNICORN_AGENT_HOST_TO_FILE):
        # communication with server based on unicorn
        if Unicorn is None:
            self.Unicorn = Service('unicorn', endpoints=[[defaults.LOCATOR_HOST, defaults.LOCATOR_PORT]], timeout=5)
        else:
            self.Unicorn = Unicorn
        # node-service of cocaine used to list/stop/start applications
        if Node is None:
            self.Node = Service('node', endpoints=[[defaults.LOCATOR_HOST, defaults.LOCATOR_PORT]], timeout=5)
        else:
            self.Node = Node
        # agent's datacenter name (from command line / comfiguration)
        self.DataCenter = DataCenter
        # agent's full qualified domain name will be stored here
        self.FQDN = FQDN
        self.Unicorn = Unicorn
        self.Node = Node
        # RO flag, do not start/stop applications while set
        self.DryRun = DryRun
        # path to agent's lock in unicorn
        self.UnicornAgentLockFile = UnicornAgentLockFile
        # path to file where agent sends it's own state to server (agent's "From" direction)
        self.UnicornAgentFromFile = UnicornAgentFromFile
        # path to file where agent watch state from server (agent's "To" direction)
        self.UnicornAgentToFile = UnicornAgentToFile
        # here we store state received from server
        self.PendingState = State(path=self.UnicornAgentToFile)
        # common to server and agent methods for unicorn
        self.UnicornTool = common.UnicornTool()

        self.log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'initialize'})
        # main ioloop, all coroutines are there
        self.io_loop = tornado.ioloop.IOLoop.current()

        # main IdleFlag, we may stop server without consequences if this flag is set
        self.AllowStopAll = Event()
        # we clear this flag only when we do something important, it must be set all time else
        self.AllowStopAll.set()
        # main flag is set only when agent's lock is acquired, all coroutines waits it on start
        self.AllowStartAll = Event()

        self.log.debug('UnicornAgentLockFile: %s', UnicornAgentLockFile)

        # acquiring main lock here, waiting it forever
        # _ALL_ coroutines except this, stay blocked at entry point till AllowStartAll.set() by this coroutine
        self.io_loop.add_future(self.OrchestratorLock(Unicorn=self.Unicorn,
                                                      LockName=self.UnicornAgentLockFile,
                                                      EventOnSuccess=self.AllowStartAll),
                                lambda x: [self.log.critical("Global lock exited. BUG!"), self.ScheduleStop(None)])
        # we need to send state to server periodically to stay in "active" state,
        # if we don't, server will treat this certain agent as "inactive"
        self.io_loop.add_future(self.HeartBeat(Seconds=const.UNICORN_HEARTBEAT_SLEEP_DURATION,
                                               EventSet=self.PendingState.UpdateTrigger),
                                lambda x: [self.log.critical("HeartBeat exited. BUG!"), self.ScheduleStop(None)])
        # watch state-file server will send to us
        self.io_loop.add_future(self.StateWatcher(Unicorn=self.Unicorn,
                                                  UnicornAgentToFile=self.UnicornAgentToFile,
                                                  StoreInObj=self.PendingState,
                                                  EventSetOnUpdate=self.PendingState.UpdateTrigger,
                                                  EventWaitFor=self.AllowStartAll),
                                lambda x: [self.log.critical("StateWatcher() exited. BUG!"), self.ScheduleStop(None)])
        # main coroutine, sets/clears flag IdleFlag during important operations
        # reads changes from PendingState each iteration
        self.io_loop.add_future(self.Agent(DataCenter=self.DataCenter,
                                           FQDN=self.FQDN,
                                           Unicorn=self.Unicorn,
                                           UnicornAgentFromFile=UnicornAgentFromFile,
                                           Node=self.Node,
                                           PendingState=self.PendingState,
                                           EventWaitFor=self.AllowStartAll,
                                           IdleFlag=self.AllowStopAll,
                                           DryRun=self.DryRun),
                                lambda x: [self.log.critical("Agent() exited. BUG!"), self.ScheduleStop(None)])

    def SplitByGroups(self, Items, Size=10):
        """
        Break an iterable into lists of a given length.

        >>> list(chunked([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]))
        [[1, 2, 3, 4, 5, 6, 7, 8, 9, 10], [11]]
        >>> list(chunked([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11], 3))
        [[1, 2, 3], [4, 5, 6], [7, 8, 9], [10, 11]]
        """
        return (Items[i:i + Size] for i in xrange(0, len(Items), Size))

    def SignalHandler(self, signum, frame):
        log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'signal-handler'})
        if signum in [signal.SIGINT, signal.SIGTERM]:
            log.info('Signal -%d catched. Scheduling exit...', signum)
            try:
                tornado.ioloop.IOLoop.current().add_callback_from_signal(lambda: self.ScheduleStop(self.AllowStopAll))
            except:
                log.critical('Trace:\n', exc_info=True)
        if signum in [signal.SIGHUP]:
            log.info('Signal -%d catched. State update is not implemented yet!', signum)
        if signum in [signal.SIGUSR1]:
            log.warning('Signal -%d catched and appearing of this message is only one reaction to it.', signum)

    @gen.coroutine
    def ScheduleStop(self, event):
        log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'shutdown'})
        if event is not None:
            self.io_loop.add_future(event.wait(),
                                    lambda x: [tornado.ioloop.IOLoop.instance().stop(), log.info('Stopped.')])
        else:
            self.io_loop.add_future(tornado.ioloop.IOLoop.instance().stop(),
                                    lambda x: log.info('Exited immediately without cleanup.'))

    @gen.coroutine
    def StateWatcher(self, Unicorn, UnicornAgentToFile, StoreInObj, EventSetOnUpdate, EventWaitFor):
        log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'state-watcher'})
        log.debug('Started. Waiting for global lock.')
        yield EventWaitFor.wait()
        log.debug('Cleaning up outdated InputState in unicorn.')
        try:
            yield self.UnicornTool.UNodeStore(Unicorn=Unicorn,
                                              UNodePath=UnicornAgentToFile,
                                              Value='{}',
                                              UTimeOut=const.UNICORN_TIMEOUT)
        except:
            log.critical('Trace:\n', exc_info=True)
            log.critical('Failed to cleaup InputState in unicorn! Exiting.')
            self.ScheduleStop(None)
        else:
            log.debug('InputState is clean now.')
            yield self.UnicornTool.UNodeWatcher(Unicorn=Unicorn,
                                                UNodePath=UnicornAgentToFile,
                                                StoreInObj=StoreInObj,
                                                EventSetOnUpdate=EventSetOnUpdate,
                                                RoutineName='state-watcher')

    @gen.coroutine
    def OrchestratorLock(self, Unicorn, LockName, EventOnSuccess, TimeOut=None):
        log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'unicorn-lock'})
        try:
            log.info('Starting orchestrator agent.')
            log.info('Trying to get lock in unicorn...')
            log.debug('Using lock %s', LockName)
            glock = yield Unicorn.lock(LockName)
            yield glock.rx.get(timeout=TimeOut)
        except:
            log.debug("Exception:\n", exc_info=True)
            try:
                yield glock.tx.close()
                log.debug('Timeout reached, can not get lock within %d seconds', TimeOut)
            except:
                log.critical("Channel to unicorn failed.", exc_info=True)
            finally:
                self.ScheduleStop(None)
        else:
            log.info('Lock acquired: %s', LockName)
            EventOnSuccess.set()
            while True:
                yield gen.sleep(const.AGENT_SLEEP_SECONDS)

    @gen.coroutine
    def Agent(self,
              DataCenter,
              FQDN,
              Unicorn,
              UnicornAgentFromFile,
              Node,
              PendingState,
              EventWaitFor,
              IdleFlag,
              DryRun,
              NodeTimeOut=const.NODE_TIMEOUT):
        log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'agent'})
        log.debug('Started. Waiting for global lock.')
        yield EventWaitFor.wait()
        CurrentState = State(path=UnicornAgentFromFile)
        log.debug('Creating initial state.')
        try:
            yield self.FillState(StateStoreIn=CurrentState,
                                 Node=Node,
                                 DataCenter=DataCenter,
                                 FQDN=FQDN)
        except:
            log.critical('Failed to create initial state! Exiting.')
            log.critical('Trace:\n', exc_info=True)
            self.ScheduleStop(None)
        prev_apps = CurrentState.state[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP].keys()
        try:
            yield self.UploadState(SourceState=CurrentState,
                                   Unicorn=Unicorn,
                                   UnicornAgentFromFile=UnicornAgentFromFile)
        except:
            log.critical('Failed to upload initial state! Exiting.')
            log.critical('Trace:\n', exc_info=True)
            self.ScheduleStop(None)
        else:
            log.debug('Initial state created.')
        PendingStateLastVersion = PendingState.version
        while True:
            PendingState.UpdateTrigger.clear()
            try:
                yield self.FillState(StateStoreIn=CurrentState,
                                     Node=Node,
                                     DataCenter=DataCenter,
                                     FQDN=FQDN)
            except:
                log.critical('Failed to fill in state! Exiting.')
                log.critical('Trace:\n', exc_info=True)
                self.ScheduleStop(None)
            cstate = CurrentState.state
            try:
                yield self.UploadState(SourceState=CurrentState,
                                       Unicorn=Unicorn,
                                       UnicornAgentFromFile=UnicornAgentFromFile)
            except:
                log.critical('Failed to upload state! Exiting.')
                log.critical('Trace:\n', exc_info=True)
                self.ScheduleStop(None)
            log.debug('Waiting for server answer or internal heartbeat for %d seconds. Whichever come first.', const.UNICORN_HEARTBEAT_SLEEP_DURATION)
            yield PendingState.UpdateTrigger.wait()
            log.debug('It is time to wake up after event PendingState.UpdateTrigger was set.')
            pstate = PendingState.state
            if PendingStateLastVersion == PendingState.version:
                log.debug('Current state is up to date.')
            else:
                try:
                    log.debug('There is a new state to process.')
                    IdleFlag.clear()
                    if pstate[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP] is not None:
                        papps = pstate[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP].keys()
                        capps = cstate[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP].keys()
                        napps = set(capps) - set(prev_apps)
                        stop_diff = set(capps) - set(papps)
                        start_diff = set(papps) - set(capps)
                        log.debug('Server provided new state, we should stop apps: %s', stop_diff)
                        log.debug('... we should start apps: %s', start_diff)
                        if len(stop_diff) > 0:
                            for app in stop_diff:
                                if app in napps:
                                    log.info('Detected new application: %s.', app)
                                else:
                                    if not DryRun:
                                        try:
                                            timeout = datetime.timedelta(seconds=NodeTimeOut)
                                            channel = yield gen.with_timeout(timeout, Node.pause_app(app))
                                            yield gen.with_timeout(timeout, channel.rx.get())
                                        except:
                                            log.critical('Failed to stop application: %s', app, exc_info=True)
                                        else:
                                            log.info('Application stopped: %s', app)
                                    else:
                                        log.info('Dry run enabled, do not call stop for app %s.', app)
                        if len(start_diff) > 0:
                            for app in start_diff:
                                if not DryRun:
                                    try:
                                        timeout = datetime.timedelta(seconds=NodeTimeOut)
                                        profile = pstate[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP][app][const.UNICORN_COMMON_HOST_STATE_JSON_KEY_PROFILE_TREE][const.UNICORN_COMMON_HOST_STATE_JSON_KEY_PROFILE_NAME]
                                        channel = yield gen.with_timeout(timeout, Node.start_app(app, profile))
                                        yield gen.with_timeout(timeout, channel.rx.get())
                                    except:
                                        log.critical('Failed to start application: %s', app, exc_info=True)
                                    else:
                                        log.info('Application started: %s', app)
                                else:
                                    log.info('Dry run enabled, do not call start for app %s.', app)
                        prev_apps = capps
                    else:
                        log.debug('pstate[app] is: %s', pstate[const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP])
                except:
                    log.critical('Trace:\n', exc_info=True)
                finally:
                    IdleFlag.set()
                PendingStateLastVersion = PendingState.version
#            yield gen.sleep(const.AGENT_SLEEP_SECONDS)
#            log.debug('Slept %s second(s).', const.AGENT_SLEEP_SECONDS)

    @gen.coroutine
    def FillState(self,
                  StateStoreIn,
                  Node,
                  DataCenter,
                  FQDN,
                  NodeTimeOut=const.NODE_TIMEOUT):
        log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'state-updater'})
        try:
            n = yield Node.list(NodeTimeOut)
        except:
            log.critical("Connection to service locator failed!")
            raise EOFError('Connection to service locator failed!')
        else:
            try:
                apps = {}
                app_list = yield n.rx.get(NodeTimeOut)
#                next command hangs forever and I don't know why
#                yield n.tx.close()
            except:
                log.critical("node.list() failed!")
                raise EOFError('node.list() failed!')
            else:
                try:
                    timeout = datetime.timedelta(seconds=NodeTimeOut)
                    on_timeout_reply = {
                        "error": "info was timeouted",
                        "state": "unresponsive",
                        "meta": "the error was generated orchestrator agent"
                    }
                    for names in self.SplitByGroups(Items=app_list, Size=25):
                        futures = {}
                        for app in names:
                            res_future = (yield Node.info(app, 0x1)).rx.get()
                            futures[app] = gen.with_timeout(timeout, res_future)
                        wait_iterator = gen.WaitIterator(**futures)
                        while not wait_iterator.done():
                            try:
                                info = yield wait_iterator.next()
                                apps[wait_iterator.current_index] = info
                            except gen.TimeoutError:
                                apps[wait_iterator.current_index] = on_timeout_reply
                            except Exception as err:
                                apps[wait_iterator.current_index] = str(err)
                                log.critical('Trace:\n', exc_info=True)
                    StateStoreIn.state.update({const.UNICORN_COMMON_HOST_STATE_JSON_KEY_APP: apps,
                                               const.UNICORN_AGENT_HOST_STATE_JSON_KEY_DC: DataCenter,
                                               const.UNICORN_AGENT_HOST_STATE_JSON_KEY_FQDN: FQDN})
                    log.debug('Object State updated using apps from nodeservice')
                except:
                    log.critical('Trace:\n', exc_info=True)
                    raise EOFError('Error while fulfilling the state with fresh data from nodeservice!')
        log.debug('SendState finished.')


    @gen.coroutine
    def UploadState(self,
                    SourceState,
                    Unicorn,
                    UnicornAgentFromFile=const.UNICORN_AGENT_HOST_FROM_FILE,
                    UnicornTimeOut=const.UNICORN_TIMEOUT):
        log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'state-uploader'})
        # uploading state to unicorn
        try:
            # we need to know a version number
            u = yield Unicorn.get(UnicornAgentFromFile)
            result = yield u.rx.get(timeout=const.UNICORN_TIMEOUT)
            yield u.tx.close()
        except:
            exists = False
        else:
            exists = result[0] is not None
        if exists:
            # state already exists
            log.debug('State "%s" already exists in unicorn.', UnicornAgentFromFile)
            state_str = yield SourceState.caststr()
            try:
                u = yield Unicorn.put(UnicornAgentFromFile, state_str, result[1])
                result = yield u.rx.get(UnicornTimeOut)
                log.info('State uploaded to Unicorn.')
            except:
                log.critical('Can not write new state to unicorn!', exc_info=True)
                raise EOFError('Failed to upload state to unicorn!')
            finally:
                yield u.tx.close()
        else:
            # state doesn't exist
            log.debug('State "%s" not found in unicorn. It will be created.', UnicornAgentFromFile)
            state_str = yield SourceState.caststr()
            try:
                u = yield Unicorn.create(UnicornAgentFromFile, state_str)
                result = yield u.rx.get(UnicornTimeOut)
            except:
                log.critical("Service unicorn failed.", exc_info=True)
                raise EOFError('Failed to create state in unicorn!')
            else:
                log.info('State created: %s', UnicornAgentFromFile)
            finally:
                yield u.tx.close()
        log.debug('SendState finished.')

    @gen.coroutine
    def HeartBeat(self, Seconds, EventSet):
        while True:
            yield gen.sleep(Seconds)
            EventSet.set()


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--host", type=str, default=defaults.LOCATOR_HOST, help="locator-service host")
    p.add_argument("--port", type=int, default=defaults.LOCATOR_PORT, help="locator-service port")
    p.add_argument("--fqdn", type=str, default=defaults.AGENT_FQDN, help="use this fqdn as agent name in announces to orchestrator server")
    p.add_argument("--datacenter", required=True, type=str, default=defaults.AGENT_DATACENTER, help="use this as location in announces to orchestrator server")
    # this option is useless right now, but it make sense in the near future without enforce to change startup-scripts
    p.add_argument("--daemonize", required=True, action='store_true', help="run as archestrator service")
    p.add_argument("--dryrun", action='store_true', help="do not stop/start applications, useful for test purposes")
    p.add_argument("--debug", action='store_true', help="enable debug mode and print all messages to stderr")
    p.add_argument("--logfile", type=str, default=None, help="path to logfile")
    argv = p.parse_args()

    if argv.debug:
        loglevel = logdef.LOGLEVEL_DEBUG
    else:
        loglevel = logdef.LOGLEVEL_DEFAULT
    logging.basicConfig(format=logdef.LOG_FORMAT, level=loglevel, filename=argv.logfile)
    log = ContextAdapter(logging.getLogger('orchestrator'), {'routine': 'main'})

    log.debug('Using debug logging level.')
    log.debug('Using locator: %s:%d', argv.host, argv.port)

    UnicornAgentLockFile = (const.UNICORN_AGENTS_TREE + const.UNICORN_AGENT_HOST_PATTERN + const.UNICORN_SUFFIX_LOCK) % argv.fqdn
    UnicornAgentToFile = (const.UNICORN_AGENT_HOST_TO_ROOT + const.UNICORN_AGENT_HOST_PATTERN + const.UNICORN_SUFFIX_STATE) % argv.fqdn
    UnicornAgentFromFile = (const.UNICORN_AGENT_HOST_FROM_ROOT + const.UNICORN_AGENT_HOST_PATTERN + const.UNICORN_SUFFIX_STATE) % argv.fqdn

    log.debug('Using FQDN: %s', argv.fqdn)
    log.debug('Using DataCenter: %s', argv.datacenter)

    unicorn = Service('unicorn', endpoints=[[argv.host, argv.port]], timeout=const.UNICORN_TIMEOUT)
    node = Service('node', endpoints=[[argv.host, argv.port]], timeout=const.NODE_TIMEOUT)

    if argv.daemonize is True:
        log.debug('Daemon mode.')
        try:
            agent = Agent(DataCenter=argv.datacenter,
                          FQDN=argv.fqdn,
                          Unicorn=unicorn,
                          Node=node,
                          DryRun=argv.dryrun,
                          UnicornAgentLockFile=UnicornAgentLockFile,
                          UnicornAgentToFile=UnicornAgentToFile,
                          UnicornAgentFromFile=UnicornAgentFromFile)
            signal.signal(signal.SIGINT, agent.SignalHandler)
            signal.signal(signal.SIGTERM, agent.SignalHandler)
            signal.signal(signal.SIGHUP, agent.SignalHandler)
            signal.signal(signal.SIGUSR1, agent.SignalHandler)
            log.info('Starting orchestrator agent.')
            tornado.ioloop.IOLoop.current().start()
            log.critical('Main IOLoop finished.')
        except:
            log.critical('Something goes wrong.\n\n', exc_info=True)


# entry point
if __name__ == '__main__':
    try:
        main()
    except:
        exit(1)

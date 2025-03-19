"""
Starts Arbiter to facilitate container-host intercommunication,
defines dispatcher class which performs actual changes in the
testing environment.
"""
import copy
import logging
import os

from . import arbiter, docker, utils


@utils.env_stage('auxillary', fail=True)
def start_wait(state, conf):
    """
    Start arbiter and wait for its termination
    (forever, as it never terminates)
    """
    listener = _start(state, conf)
    try:
        listener.join()
    except KeyboardInterrupt:
        logging.warning('terminating arbiter')
        listener.stop()


@utils.env_stage('stop', fail=False)
def stop(state, **_):
    """
    Stop Arbiter
    """
    if state and state.get('arbiter'):
        state['arbiter'].stop()


@utils.env_stage('start', fail=True)
def start(state, conf):
    """
    Start and set state.
    """
    listener = _start(state, conf)
    state['arbiter'] = listener


def _start(state, conf):
    """
    Prepare file-based communication:
    1. Create command and progress log files
    2. Instantiate dispatcher class
    3. Start arbiter thread.
    """
    assert None not in (state, conf), \
        'state and conf must not be None'

    basedir = '{0}/arbiter'.format(conf.get('staging_dir', 'staging'))
    command_file = '{0}/commands.txt'.format(basedir)
    status_file = '{0}/status.txt'.format(basedir)
    os.makedirs(basedir, exist_ok=True)

    dispatcher = EnvDispatcher(state, conf)

    listener = arbiter.ArbiterServer(
        dispatcher=dispatcher,
        command_file=command_file,
        status_file=status_file,
    )
    listener.start()
    return listener


class EnvDispatcher:
    """
    Configures and reloads environment on demand
    """

    def __init__(self, state, conf):
        for arg in (state, conf):
            assert arg is not None, '%s must not be None' % arg
        self._state = state
        self._conf = conf
        self._docker = docker.DOCKER_API

    def _invalid(self, command=None, **kwargs):
        """
        Catch-all default method
        """
        raise NotImplementedError('an invalid command requested: {0}({1})'.format(command, kwargs))

    def _exec_steps(self, conf, steps):
        """
        Import and execute functions defined in steps
        """
        if not isinstance(conf, dict):
            raise TypeError('conf arg must be a dict')
        if not isinstance(steps, (list, tuple)):
            raise TypeError('steps arg must be a list or a tuple')

        callbacks = []
        for step in steps:
            try:
                callbacks.append(utils.importobj(step))
            # if incorrect module name is passed, we get
            # ValueError: not enough values to unpack (expected 2, got 1)
            except (ValueError, ImportError):
                raise ValueError('unable to import {0}'.format(step))

        logging.info('steps called externally: %s', steps)
        for step in callbacks:
            step(state=self._state, conf=conf)

    def run_command(self, command, args):
        """
        Determines an appropriate method and calls it.
        """
        assert isinstance(args, dict), 'args must be a dict'
        assert isinstance(command, str), 'command must be a string'

        try:
            fun = getattr(self, command, None)
            if not callable(fun):
                return self._invalid(command=command, **args)
            return fun(**args)
        # Need to catch ValueError-s as they are suppressed in the caller.
        except ValueError as exc:
            raise RuntimeError(exc)

    def update_and_exec(self, conf, steps, persist=False):
        """
        Update configuration and perform steps.
        """
        if not isinstance(conf, dict):
            raise TypeError('conf arg must be a dict')
        new_conf = self._conf
        if not persist:
            new_conf = copy.deepcopy(self._conf)
        new_conf.update(conf)
        return self._exec_steps(conf=new_conf, steps=steps)

    def merge_and_exec(self, conf, steps, persist=False):
        """
        Merge configurations and perform steps.
        """
        if not isinstance(conf, dict):
            raise TypeError('conf arg must be a dict')
        new_conf = self._conf
        if not persist:
            new_conf = copy.deepcopy(self._conf)
        utils.merge(original=new_conf, update=conf)
        return self._exec_steps(conf=new_conf, steps=steps)

    def container_info(self, name):
        """
        Get all container attributes
        """
        container = None
        for candidate in self._docker.containers.list():
            if candidate.name == name:
                container = candidate
        return container.attrs

    def container_list(self, filters, show_all=True):
        """
        Return a list of available container fqdn-s
        """
        conts = self._docker.containers.list(filters=filters, all=show_all)
        return [cont.name for cont in conts]

    def container_remove(self, name):
        """
        Remove a container given a name
        """
        # A bug in Docker API, perhaps, some splitting artifacts.
        # exact matches work only if prepended with a wildcard for
        # one char (.?)
        name_regex = '^.?{name}.{net}$'.format(
            name=name,
            net=self._conf.get('network_name', ''),
        )
        conts = self._docker.containers.list(
            filters={'name': name_regex},
            all=True,
        )
        for cont in conts:
            cont.remove(force=True)

    def container_stop(self, name):
        """
        Stop a container given a name
        """
        # A bug in Docker API, perhaps, some splitting artifacts.
        # exact matches work only if prepended with a wildcard for
        # one char (.?)
        name_regex = '^.?{name}.{net}$'.format(
            name=name,
            net=self._conf.get('network_name', ''),
        )
        conts = self._docker.containers.list(
            filters={'name': name_regex},
            all=True,
        )
        for cont in conts:
            cont.stop()

    def container_put(self, name, path, content, mode='0640'):
        """
        Put a file into container
        """
        return docker.put_file(name, path, content, int(mode, 8))

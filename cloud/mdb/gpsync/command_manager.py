from . import helpers


_substitutions = {
    'pgdata': '%p',
    'master_host': '%m',
    'timeout': '%t',
    'argument': '%a',
}


@helpers.decorate_all_class_methods(helpers.func_name_logger)
class CommandManager:
    def __init__(self, config):
        self._config = config

    def _prepare_command(self, command_name, **kwargs):
        command = self._config.get('commands', command_name)
        for (arg_name, arg_value) in kwargs.items():
            command = command.replace(_substitutions[arg_name], str(arg_value))
        return command

    def _exec_command(self, command_name, **kwargs):
        command = self._prepare_command(command_name, **kwargs)
        return helpers.subprocess_call(command)

    def promote(self, pgdata):
        return self._exec_command('promote', pgdata=pgdata)

    def rewind(self, pgdata, master_host):
        return self._exec_command('rewind', pgdata=pgdata, master_host=master_host)

    def get_control_parameter(self, pgdata, parameter, preproc=None, log=True):
        command = self._prepare_command('get_control_parameter', pgdata=pgdata, argument=parameter)
        res = helpers.subprocess_popen(command, log_cmd=log)
        if not res:
            return None
        value = res.communicate()[0].decode('utf-8').split(':')[-1].strip()
        if preproc:
            return preproc(value)
        else:
            return value

    def list_clusters(self, log=True):
        command = self._prepare_command('list_clusters')
        res = helpers.subprocess_popen(command, log_cmd=log)
        if not res:
            return None
        output, _ = res.communicate()
        return output.decode('utf-8').rstrip('\n').split('\n')

    def start_postgresql(self, timeout, pgdata):
        return self._exec_command('pg_start', timeout=timeout, pgdata=pgdata)

    def stop_postgresql(self, timeout, pgdata):
        return self._exec_command('pg_stop', timeout=timeout, pgdata=pgdata)

    def get_postgresql_status(self, pgdata):
        return self._exec_command('pg_status', pgdata=pgdata)

    def reload_postgresql(self, pgdata):
        return self._exec_command('pg_reload', pgdata=pgdata)

    def start_pgbouncer(self):
        return self._exec_command('bouncer_start')

    def stop_pgbouncer(self):
        return self._exec_command('bouncer_stop')

    def get_pgbouncer_status(self):
        return self._exec_command('bouncer_status')

    def generate_recovery_conf(self, filepath, master_host):
        return self._exec_command('generate_recovery_conf', pgdata=filepath, master_host=master_host)

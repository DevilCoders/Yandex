import logging
import re
import requests


class ClusterMaster(object):
    def __init__(self, master_host_port):
        """ Init with 'master:port' string.
        """

        self.base_url = 'http://' + master_host_port

    @staticmethod
    def _get_text_or_none(url, **kwargs):
        try:
            req = requests.get(url, **kwargs)
            logging.debug("GET {} {}".format(url, req.status_code))
            if req.status_code == 200:
                return req.text
            return None
        except Exception:
            return None

    @staticmethod
    def _target_url_path(target, worker):
        if worker is None:
            return target
        return "{}/{}".format(target, worker)

    def get_target_text_state(self, target, worker=None):
        """ By default get all info about target on all workers.
        If worker is set, get info about the target on this worker only.
        """

        formed_url = "{}/target_text/{}".format(self.base_url, self._target_url_path(target, worker))
        return self._get_text_or_none(formed_url, timeout=2)

    def _parse_target_common_state_line(self, line):
        keys = ('host', 'worker_status', 'target_status', 'last_started', 'last_finished',
                'last_success', 'last_failure', 'act', 'ttl')
        return dict(zip(keys, re.split('\s+', line.strip())))

    def _parse_target_worker_state_line(self, line):
        keys = ('task', 'target_status', 'last_started', 'last_finished',
                'last_success', 'last_failure')
        return dict(zip(keys, re.split('\s+', line.strip())[:-1]))

    def get_target_dict_state(self, target_name, worker=None):
        """ Gets target text state and parses it to dictionary.
        For target it is host(worker) -> worker_info
        For worker it is workers_task -> task_info
        """

        text = self.get_target_text_state(target_name, worker)
        if text is None:
            return None

        state_dict = {}
        for line in text.strip().split('\n'):
            if worker is None:
                worker_dict = self._parse_target_common_state_line(line)
                state_dict[worker_dict.pop('host')] = worker_dict
            else:
                task_dict = self._parse_target_worker_state_line(line)
                state_dict[task_dict.pop('task')] = task_dict
        return state_dict

    def get_target_times(self, target):
        """ Get all times of specified target.
        Returns dict, were keys are: started, finished, success, failure.
        """

        formed_url = '{}/targettimes/{}'.format(self.base_url, target)
        answer = self._get_text_or_none(formed_url, timeout=2)
        if answer is None:
            return None
        parsed_times = {}
        for item in answer.strip().split(','):
            key, val = item.split('=')
            parsed_times[key] = int(val)
        return parsed_times

    def get_last_started_time(self, target, worker=None):
        """ Get target's last started time. Worker can be specified.
        """

        if worker:
            tasks_dict = self.get_target_dict_state(target, worker)
            time = max([info['last_started'] for info in tasks_dict.values()]) if tasks_dict else None
            return time
        times = self.get_target_times(target)
        return times if times is None else times['started']

    def get_last_finished_time(self, target, worker=None):
        """ Get target's last finished time. Worker can be specified.
        """

        if worker:
            tasks_dict = self.get_target_dict_state(target, worker)
            time = max([info['last_finished'] for info in tasks_dict.values()]) if tasks_dict else None
            return time
        times = self.get_target_times(target)
        return times if times is None else times['finished']

    def get_last_success_time(self, target, worker=None):
        """ Get target's last success time. Worker can be specified.
        """

        if worker:
            tasks_dict = self.get_target_dict_state(target, worker)
            time = max([info['last_success'] for info in tasks_dict.values()]) if tasks_dict else None
            return time
        times = self.get_target_times(target)
        return times if times is None else times['success']

    def get_last_failure_time(self, target, worker=None):
        """ Get target's last failure time. Worker can be specified.
        """

        if worker:
            tasks_dict = self.get_target_dict_state(target, worker)
            time = max([info['last_success'] for info in tasks_dict.values()]) if tasks_dict else None
            return time
        times = self.get_target_times(target)
        return times if times is None else times['failure']

    def get_target_text_status(self, target, worker=None):
        """ Returns target text repr for set of current statuses on all workers if
        worker is None, otherwise returns text repr for set of statuses for all
        worker's tasks.

        This is a simple check. Good to check status of targets with simple
        workers(with one task). If you want more info, try to use
        'get_target_text/dict_state' methods.
        """

        url = "{}/targetstatus/{}".format(self.base_url, self._target_url_path(target, worker))
        return self._get_text_or_none(url, timeout=2)

    def get_target_status_count(self, target, status, worker=None):
        """ Calcs given status count for all workers of target(if worker is None)
        or for all worker's tasks
        """

        info_dict = self.get_target_dict_state(target, worker)
        if not info_dict:
            return None
        return len(filter(lambda info: info['target_status'] == status, info_dict.itervalues()))

    def get_target_status_percentage(self, target, status, worker=None):
        """ Calcsi precentage of given status for all workers of target(if worker is None)
        or for all workers tasks
        """

        workers_dict = self.get_target_dict_state(target, worker)
        if not workers_dict:
            return None
        return self.get_target_status_count(target, status, worker) * 100.0 / len(workers_dict)

    def _command(self, cmd, params=None):
        formed_url = "{}/command/{}".format(self.base_url, cmd)
        if params is not None:
            formed_url = "{}?{}".format(formed_url, "&".join(params))
        req = requests.get(formed_url, timeout=2)
        logging.debug("GET {} {}".format(formed_url, req.status_code))
        return req

    def _worker_command(self, cmd, target, worker=None, task=None, calltype="this_only", other_params=[]):
        """ Valid calltypes: this_only/path/path_on_all_workers/followers/followers_on_all_workers
        """

        params = ["target=%s" % target]
        if worker is not None:
            params.append("worker=%s" % worker)
        if task is not None:
            params.append("task=%s" % task)
        if calltype == "path" or calltype == "path_on_all_workers":
            cmd += "-path"
        elif calltype == "followers" or calltype == "followers_on_all_workers":
            cmd += "-following"
        if calltype == "path_on_all_workers" or calltype == "followers_on_all_workers":
            params.append("all_workers")
        params.extend(other_params)
        return self._command(cmd, params)

    def run(self, target, worker=None, task=None, calltype="this_only"):
        """Run target(or target on worker and task if they are given).
        Valid calltypes are: this_only/path/path_on_all_workers
        """
        return self._worker_command("run", target, worker, task, calltype)

    def retry_run(self, target, worker=None, task=None, calltype="path"):
        """Retry target(or target on worker and task if they are given).
        Valid calltypes are: path/path_on_all_workers
        """
        return self._worker_command("retry-run", worker, task, calltype)

    def forced_run(self, target, worker=None, task=None):
        """Forced run target(or target on worker and task if they are given).
        """
        return self._worker_command("forced-run", target, worker, task)

    def forced_ready(self, target, worker=None, task=None):
        """Forced set ready target(or target on worker and task if they are given).
        """
        return self._worker_command("forced-ready", target, worker, task)

    def cancel(self, target, worker=None, task=None, calltype="this_only"):
        """Cancel target(or target on worker and task if they are given).
        Valid calltypes are: this_only/path/followers/path_on_all_workers/
        followers_on_all_workers
        """
        return self._worker_command("cancel", target, worker, task, calltype)

    def invalidate(self, target, worker=None, task=None, calltype="this_only"):
        """invalidate target(or target on worker and task if they are given).
        Valid calltypes are: this_only/path/followers/path_on_all_workers/
        followers_on_all_workers
        """
        return self._worker_command("invalidate", target, worker, task, calltype)

    def reset(self, target, worker=None, task=None, calltype="this_only"):
        """Reset target(or target on worker and task if they are given).
        Valid calltypes are: this_only/path/followers/path_on_all_workers/
        followers_on_all_workers
        """
        return self._worker_command("reset", target, worker, task, calltype)

    def mark_succes(self, target, worker=None, task=None):
        """Mark target as success(or target on worker and task if they are given).
        """
        return self._worker_command("mark-succes", target, worker, task)

    def reset_stat(self, target, worker=None, task=None):
        """Reset target's stat(or target's on worker and task if they are given).
        """
        return self._worker_command("reset-stat", target, worker, task)

    def show_with_children(self, target, worker=None, task=None):
        """Show targets graph(walkin down)(or target on worker and task if they are given).
        """
        params = ["walk=up", "depth=-1"]
        return self._worker_command("reset-stat", target, worker, task, other_params=params)

    def show_with_parents(self, target, worker=None, task=None):
        """Show targets graph(walkin up)(or target on worker and task if they are given).
        """
        params = ["walk=down", "depth=-1"]
        return self._worker_command("reset-stat", target, worker, task, other_params=params)

    def set_var(self, var_name, var_value, worker=None):
        """ Set variables. Worker can be specified.
        """

        if worker:
            url_path = 'variables/set?name={}&value={}&worker={}'.format(var_name, var_value, worker)
        else:
            url_path = 'variables/set?name={}&value={}'.format(var_name, var_value)

        formed_url = "{}/{}".format(self.base_url, url_path)
        req = requests.get(formed_url, timeout=2)
        logging.debug("GET {} {}".format(formed_url, req.status_code))
        return req

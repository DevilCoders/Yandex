# -*- coding: utf-8 -*-
#
'''
Custom Windows Task Scheduler Module
Improved version of info function, returing 

.
'''
# Import Python libs
from __future__ import absolute_import, print_function, unicode_literals
from datetime import datetime
import logging
import time

# Import Salt libs
import salt.utils.platform
import salt.utils.winapi

# taking a bunch of stuff from standard win_task module to satisfy dependencies
from salt.modules.win_task import (
    list_tasks,
    _get_date_value,
    _reverse_lookup,
    results,
    states,
    duration,
    instances,
    action_types,
    trigger_types,
)

# Import 3rd Party Libraries
try:
    import pythoncom
    import win32com.client

    HAS_DEPENDENCIES = True
except ImportError:
    HAS_DEPENDENCIES = False
from salt.ext.six.moves import range

log = logging.getLogger(__name__)

# Define the module's virtual name
__virtualname__ = 'mdb_win_task'


def __virtual__():
    '''
    Only works on Windows systems
    '''
    if salt.utils.platform.is_windows():
        if not HAS_DEPENDENCIES:
            log.warning('Could not load dependencies for %s', __virtualname__)
        return __virtualname__
    return False, 'Module win_task: module only works on Windows systems'


def task_info(name, location='\\'):
    r'''
    Get the details about a task in the task scheduler.

    Args:

        name (str):
            The name of the task for which to return the status

        location (str):
            A string value representing the location of the task. Default is
            ``\`` which is the root for the task scheduler
            (``C:\Windows\System32\tasks``).

    Returns:
        dict: A dictionary containing the task configuration

    CLI Example:

    .. code-block:: bash

        salt 'minion-id' task.info <task_name>
    '''
    # Check for existing folder
    if name not in list_tasks(location):
        return '{0} not found in {1}'.format(name, location)

    # connect to the task scheduler
    with salt.utils.winapi.Com():
        task_service = win32com.client.Dispatch("Schedule.Service")
    task_service.Connect()

    # get the folder to delete the folder from
    task_folder = task_service.GetFolder(location)
    task = task_folder.GetTask(name)

    properties = {
        'enabled': task.Enabled,
        'last_run': _get_date_value(task.LastRunTime),
        'last_run_result': results.get(task.LastTaskResult),
        'missed_runs': task.NumberOfMissedRuns,
        'next_run': _get_date_value(task.NextRunTime),
        'status': states[task.State],
    }

    def_set = task.Definition.Settings

    settings = {'allow_demand_start': def_set.AllowDemandStart, 'force_stop': def_set.AllowHardTerminate}

    if def_set.DeleteExpiredTaskAfter == '':
        settings['delete_after'] = False
    elif def_set.DeleteExpiredTaskAfter == 'PT0S':
        settings['delete_after'] = 'Immediately'
    else:
        settings['delete_after'] = _reverse_lookup(duration, def_set.DeleteExpiredTaskAfter)

    if def_set.ExecutionTimeLimit == '':
        settings['execution_time_limit'] = False
    else:
        settings['execution_time_limit'] = _reverse_lookup(duration, def_set.ExecutionTimeLimit)

    settings['multiple_instances'] = _reverse_lookup(instances, def_set.MultipleInstances)

    if def_set.RestartInterval == '':
        settings['restart_interval'] = False
    else:
        settings['restart_interval'] = _reverse_lookup(duration, def_set.RestartInterval)

    if settings['restart_interval']:
        settings['restart_count'] = def_set.RestartCount
    settings['stop_if_on_batteries'] = def_set.StopIfGoingOnBatteries
    settings['wake_to_run'] = def_set.WakeToRun

    conditions = {
        'ac_only': def_set.DisallowStartIfOnBatteries,
        'run_if_idle': def_set.RunOnlyIfIdle,
        'run_if_network': def_set.RunOnlyIfNetworkAvailable,
        'start_when_available': def_set.StartWhenAvailable,
    }

    if conditions['run_if_idle']:
        idle_set = def_set.IdleSettings
        conditions['idle_duration'] = idle_set.IdleDuration
        conditions['idle_restart'] = idle_set.RestartOnIdle
        conditions['idle_stop_on_end'] = idle_set.StopOnIdleEnd
        conditions['idle_wait_timeout'] = idle_set.WaitTimeout

    if conditions['run_if_network']:
        net_set = def_set.NetworkSettings
        conditions['network_id'] = net_set.Id
        conditions['network_name'] = net_set.Name

    actions = []
    for actionObj in task.Definition.Actions:
        action = {'action_type': _reverse_lookup(action_types, actionObj.Type)}
        if actionObj.Path:
            action['cmd'] = actionObj.Path
        if actionObj.Arguments:
            action['arguments'] = actionObj.Arguments
        if actionObj.WorkingDirectory:
            action['working_dir'] = actionObj.WorkingDirectory
        actions.append(action)

    triggers = []
    for triggerObj in task.Definition.Triggers:
        trigger = {'trigger_type': _reverse_lookup(trigger_types, triggerObj.Type)}
        if triggerObj.ExecutionTimeLimit:
            trigger['execution_time_limit'] = _reverse_lookup(duration, triggerObj.ExecutionTimeLimit)
        if triggerObj.StartBoundary:
            start_date, start_time = triggerObj.StartBoundary.split('T', 1)
            trigger['start_date'] = start_date
            trigger['start_time'] = start_time
        if triggerObj.EndBoundary:
            end_date, end_time = triggerObj.EndBoundary.split('T', 1)
            trigger['end_date'] = end_date
            trigger['end_time'] = end_time
        trigger['enabled'] = triggerObj.Enabled
        if hasattr(triggerObj, 'RandomDelay'):
            if triggerObj.RandomDelay:
                trigger['random_delay'] = _reverse_lookup(duration, triggerObj.RandomDelay)
            else:
                trigger['random_delay'] = False
        if hasattr(triggerObj, 'Repetition'):
            if triggerObj.Repetition.Interval:
                trigger['repetition_interval'] = _reverse_lookup(duration, triggerObj.Repetition.Interval)
            else:
                trigger['repetition_interval'] = None
        else:
            trigger['repetition_interval'] = False
        if hasattr(triggerObj, 'DaysInterval'):
            trigger['days_interval'] = triggerObj.DaysInterval
        else:
            trigger['days_interval'] = False

        if hasattr(triggerObj, 'Delay'):
            if triggerObj.Delay:
                trigger['delay'] = _reverse_lookup(duration, triggerObj.Delay)
            else:
                trigger['delay'] = False
        triggers.append(trigger)

    properties['settings'] = settings
    properties['conditions'] = conditions
    properties['actions'] = actions
    properties['triggers'] = triggers
    ret = properties

    return ret

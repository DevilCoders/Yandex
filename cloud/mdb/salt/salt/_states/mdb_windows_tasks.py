from __future__ import absolute_import


def __virtual__():
    """
    Only load if the tasks module is present
    """
    return 'task.list_folders' in __salt__ and 'mdb_win_task.task_info' in __salt__


def present(
    name,
    command,
    repeat_interval=None,
    days_interval=False,
    enabled=True,
    arguments=None,
    schedule_type='Once',
    start_time='00:00:00',
    location='salt',
    force_stop=True,
    multiple_instances="Parallel",
    **kwargs
):
    """
    State to ensure the scheduled task exists in Windows Scheduler

    """
    # check if the location exists
    # and create the full path if necessary
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    if schedule_type == 'Daily' and days_interval == False:
        days_interval = 1

    path_exists = True
    task_exists = False
    changes = False
    changes_str = ''
    if location != '\\':
        paths = []
        current_path = ''
        folders = location.split('\\')
        current_path = folders[0]
        paths.append(current_path)
        for folder in folders[1:]:
            current_path += '\\' + folder
            paths.append(current_path)
        parent_path = '\\'
        for path in paths:
            if path not in __salt__['task.list_folders'](parent_path):
                changes_str += 'Folder {path} needs to be created at {parent_path}; '.format(
                    path=path, parent_path=parent_path
                )
                path_exists = False
                changes = True
                if not __opts__['test']:
                    __salt__['task.create_folder'](path, parent_path)
                    changes_str += 'Folder {path} has been created at {parent_path}; '.format(
                        path=path, parent_path=parent_path
                    )

    # check if task exists
    if path_exists:
        if name in __salt__['task.list_tasks'](location):
            task_exists = True
            task_props = []
            task_props = __salt__['mdb_win_task.task_info'](name, location)
            # check the properties comply
            if (
                task_props['actions'][0]['cmd'] == command
                and task_props['enabled'] == enabled
                and task_props['actions'][0]['arguments'] == arguments
                and task_props['triggers'][0]['enabled'] == True
                and task_props['triggers'][0]['start_time'] == start_time
                and task_props['triggers'][0]['trigger_type'] == schedule_type
                and task_props['triggers'][0]['days_interval'] == days_interval
                and task_props['triggers'][0]['repetition_interval'] == repeat_interval
                and task_props['settings']['multiple_instances'] == multiple_instances
                and task_props['settings']['force_stop'] == force_stop
            ):
                ret['comment'] = 'Task {0} is present'.format(name)
                return ret
            else:
                changes_str += 'Task {task} properties needs to be changed; '.format(task=name)
                changes = True
        else:
            changes_str += 'Task {task} has to be created; '.format(task=name)
            task_exists = False
            changes = True
    else:
        changes_str += 'Task {task} has to be created; '.format(task=name)

    if __opts__['test']:
        if changes:
            ret['result'] = None
            if task_exists:
                ret['changes'] = {name: 'Task have to be updated'}
            else:
                ret['changes'] = {name: 'Task have to be created'}
            ret['comment'] = changes_str
        else:
            ret['result'] = True
            ret['changes'] = {}
            ret['comment'] = ''
        return ret

    task_created = __salt__['task.create_task'](
        name,
        user_name='System',
        force=True,
        action_type='Execute',
        cmd=command,
        arguments=arguments,
        trigger_type=schedule_type,
        start_time=start_time,
        repeat_interval=repeat_interval,
        location=location,
        multiple_instances=multiple_instances,
        force_stop=force_stop,
        enabled=enabled,
    )

    if not task_created:
        ret['result'] = False
        ret['comment'] = changes_str + 'Failed to create task {0}'.format(name)
        return ret
    ret['result'] = True
    ret['comment'] = changes_str + 'Task {0} has been created'.format(name)
    ret['changes'][name] = 'Present'
    return ret

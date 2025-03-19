import logging
from datetime import datetime, timedelta
from time import sleep
from .internal_api import InternalAPI


def delete_clusters(config, clusters):
    """
    Delete database clusters using DBaaS Internal API.
    """
    if not clusters:
        return

    tasks = create_delete_tasks(config, clusters)

    await_tasks(config, tasks)


def stop_clusters(config, clusters):
    """
    Stop database clusters using DBaaS Internal API.
    """
    if not clusters:
        return

    tasks = create_stop_tasks(config, clusters)

    await_tasks(config, tasks)


def create_stop_tasks(config, clusters):
    """
    Create tasks to stop database clusters.
    """
    api = InternalAPI(config)
    tasks = {}
    for cluster in clusters:
        cid = cluster.cid
        name = cluster.name
        created = cluster.created_at.strftime('%Y-%m-%d at %H:%M')
        try:
            task_id = api.stop_cluster(cluster)
            tasks[task_id] = cluster
            logging.info(
                'Created task to stop database cluster "%s"' ' (id: %s, created: %s, task id: %s)',
                name,
                cid,
                created,
                task_id,
            )
        except Exception:
            logging.exception(
                'Failed to create task to stop database' ' cluster "%s" (id: %s, created: %s)', name, cid, created
            )
    return tasks


def create_delete_tasks(config, clusters):
    """
    Create tasks to delete database clusters.
    """
    api = InternalAPI(config)
    tasks = {}
    for cluster in clusters:
        cid = cluster.cid
        name = cluster.name
        created = cluster.created_at.strftime('%Y-%m-%d at %H:%M')
        try:
            task_id = api.delete_cluster(cluster)
            tasks[task_id] = cluster
            logging.info(
                'Created task to delete database cluster "%s"' ' (id: %s, created: %s, task id: %s)',
                name,
                cid,
                created,
                task_id,
            )
        except Exception:
            logging.exception(
                'Failed to create task to delete database' ' cluster "%s" (id: %s, created: %s)', name, cid, created
            )
    return tasks


def await_tasks(config, tasks):
    """
    Wait until tasks completion.
    """
    api = InternalAPI(config)
    deadline = datetime.now() + timedelta(seconds=config['timeout_sec'])
    exceptions = {}
    while tasks and deadline > datetime.now():
        for task_id in tasks.copy():
            cluster = tasks[task_id]
            try:
                task = api.get_task_status(task_id, cluster)
                if task.done:
                    if not task.failed:
                        logging.info('Operation on cluster "%s" (%s) finished', cluster.name, cluster.cid)
                        del tasks[task_id]
                    else:
                        logging.error(
                            'Operation on cluster "%s" (%s)' ' failed, task id: %s', cluster.name, cluster.cid, task_id
                        )
                        del tasks[task_id]

            except Exception as exc:
                exceptions[task_id] = exc
        sleep(5)

    for task_id, cluster in tasks.items():
        logging.error(
            'Operation on cluster "%s" (%s) was unable to' ' complete within %s, task_id: %s',
            cluster.name,
            cluster.cid,
            config['timeout'],
            task_id,
            exc_info=exceptions.get(task_id),
        )

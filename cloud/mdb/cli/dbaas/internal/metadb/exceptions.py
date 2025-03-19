from click import ClickException


class ClusterNotFound(ClickException):
    def __init__(self, cluster_id=None):
        message = f'Cluster {cluster_id} not found.' if cluster_id else 'Cluster not found.'
        super().__init__(message)


class SubclusterNotFound(ClickException):
    def __init__(self, subcluster_id=None):
        message = f'Subcluster {subcluster_id} not found.' if subcluster_id else 'Subcluster not found.'
        super().__init__(message)


class ShardNotFound(ClickException):
    def __init__(self, shard_id=None):
        message = f'Shard {shard_id} not found.' if shard_id else 'Shard not found.'
        super().__init__(message)


class HostNotFound(ClickException):
    def __init__(self, host=None):
        message = f'Host {host} not found.' if host else 'Host not found.'
        super().__init__(message)


class PillarNotFound(ClickException):
    def __init__(self, cluster_type=None, cluster_id=None, subcluster_id=None, shard_id=None, host=None):
        if cluster_type:
            message = f'Pillar for cluster type {cluster_type} not found.'
        elif cluster_id:
            message = f'Pillar for cluster {cluster_id} not found.'
        elif subcluster_id:
            message = f'Pillar for subcluster {subcluster_id} not found.'
        elif shard_id:
            message = f'Pillar for shard {shard_id} not found.'
        else:
            message = 'Pillar not found.'
        super().__init__(message)


class TaskNotFound(ClickException):
    def __init__(self, task_id=None):
        message = f'Task {task_id} not found.' if task_id else 'Task not found.'
        super().__init__(message)


class TaskIsInProgress(ClickException):
    def __init__(self, task_id, worker_id):
        super().__init__(f'Task {task_id} is currently executed by {worker_id}')


class MaintenanceTaskNotFound(ClickException):
    def __init__(self, task_id=None):
        message = f'Maintenance task {task_id} not found.' if task_id else 'Maintenance task not found.'
        super().__init__(message)

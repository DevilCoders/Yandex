"""
Simple mlock mock
"""

from cloud.mdb.mlock.api import lock_pb2, lock_service_pb2

from .utils import handle_action


def create_lock(state, request):
    """
    CreateLock mock
    """
    action_id = f'mlock-create-{request.id}'
    handle_action(state, action_id)
    state['mlock'][request.id] = True


def get_lock(state, request):
    """
    GetLockStatus mock
    """
    action_id = f'mlock-get-{request.id}'
    handle_action(state, action_id)
    if request.id in state['mlock']:
        res = lock_pb2.LockStatus
        res.acquired = True
        return res
    return None


def release_lock(state, request):
    """
    ReleaseLock mock
    """
    action_id = f'mlock-release-{request.id}'
    handle_action(state, action_id)
    if request.id in state['mlock']:
        del state['mlock'][request.id]
        return lock_service_pb2.ReleaseLockResponse()
    raise RuntimeError(f'Release of nonexistent lock {request.id}')


def mlock(mocker, state):
    """
    Setup mlock mock
    """
    stub = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.mlock.lock_service_pb2_grpc.LockServiceStub')
    stub.return_value.CreateLock.side_effect = lambda request, **_: create_lock(state, request)
    stub.return_value.GetLockStatus.side_effect = lambda request, **_: get_lock(state, request)
    stub.return_value.ReleaseLock.side_effect = lambda request, **_: release_lock(state, request)

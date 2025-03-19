from test.mocks import checked_run_task_with_mocks
from test.tasks.utils import check_task_interrupt_consistency
from test.mocks import get_state
from test.tasks.metastore.utils import get_state_with_pillar


def test_compute_metastore_cluster_delete_metadata_interrupt_consistency(mocker):
    """
    Check compute delete metadata interruptions
    """
    args = {'hosts': {}, 'subcid': 'subcid_1'}
    state = get_state_with_pillar(get_state())

    *_, state = checked_run_task_with_mocks(mocker, 'metastore_cluster_create', dict(**args), state=state)

    *_, state = checked_run_task_with_mocks(mocker, 'metastore_cluster_delete', args, state=state)

    check_task_interrupt_consistency(
        mocker,
        'metastore_cluster_delete_metadata',
        args,
        state,
        ignore=[],
    )

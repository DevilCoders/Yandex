from test.mocks import get_state
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency
from test.tasks.metastore.utils import get_state_with_pillar


def test_compute_metastore_cluster_create_interrupt_consistency(mocker):
    """
    Check compute create interruptions
    """

    state = get_state_with_pillar(get_state())

    check_task_interrupt_consistency(
        mocker,
        'metastore_cluster_create',
        {'hosts': {}, 'subcid': 'suya stbcid_1'},
        state,
    )


def test_metastore_cluster_create_mlock_usage(mocker):
    """
    Check create mlock usage
    """

    state = get_state_with_pillar(get_state())

    check_mlock_usage(
        mocker,
        'metastore_cluster_create',
        {'hosts': {}, 'subcid': 'subcid_1'},
        state,
    )

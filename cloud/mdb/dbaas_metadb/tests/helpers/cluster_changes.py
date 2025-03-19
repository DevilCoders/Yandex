"""
Cluster related helpers
"""

from contextlib import contextmanager

from .connect import connect
from .queries import execute_query_in_transaction, verbose_query

LOCK_CLUSTER_Q = verbose_query(
    """
    SELECT * FROM code.lock_cluster(
        i_cid          => :cid,
        i_x_request_id => :x_request_id
    )
"""
)

COMPETE_CHANGE_Q = verbose_query(
    """
    SELECT code.complete_cluster_change(
        i_cid => :cid,
        i_rev => :rev
    )
"""
)

LOCK_FUTURE_CLUSTER_Q = verbose_query(
    """
    SELECT * FROM code.lock_future_cluster(
        i_cid          => :cid,
        i_x_request_id => :x_request_id
    )
"""
)

COMPETE_FUTURE_CHANGE_Q = verbose_query(
    """
    SELECT code.complete_future_cluster_change(
        i_cid => :cid,
        i_actual_rev => :actual_rev,
        i_next_rev => :next_rev
    )
"""
)


@contextmanager
def in_transaction(context):
    """
    Start new transaction
    """
    trans = connect(context.dsn)
    try:
        yield trans
        trans.commit()
    except BaseException:
        trans.rollback()
        raise
    finally:
        trans.close()


def lock_cluster(trans, cid, x_request_id=''):
    """
    Lock cluster
    """
    return (
        execute_query_in_transaction(
            trans,
            LOCK_CLUSTER_Q,
            {
                'cid': cid,
                'x_request_id': x_request_id,
            },
        )
        .fetch()[0]
        .rev
    )


def complete_change(trans, cid, rev):
    """
    Complete cluster change
    """
    execute_query_in_transaction(trans, COMPETE_CHANGE_Q, {'cid': cid, 'rev': rev})


def lock_future_cluster(trans, cid, x_request_id=''):
    """
    Lock cluster
    """
    res = execute_query_in_transaction(
        trans,
        LOCK_FUTURE_CLUSTER_Q,
        {
            'cid': cid,
            'x_request_id': x_request_id,
        },
    ).fetch()[0]
    return res.rev, res.next_rev


def complete_future_change(trans, cid, actual_rev, next_rev):
    """
    Complete cluster change
    """
    execute_query_in_transaction(
        trans, COMPETE_FUTURE_CHANGE_Q, {'cid': cid, 'actual_rev': actual_rev, 'next_rev': next_rev}
    )


@contextmanager
def cluster_change(context):
    """
    Cluster change helper
    """
    with in_transaction(context) as trans:
        rev = lock_cluster(trans, context.cid)
        yield trans, rev
        complete_change(trans, context.cid, rev)

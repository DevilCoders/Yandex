SELECT code.release_task(
    i_worker_id => %(worker_id)s,
    i_context   => %(context)s,
    i_task_id   => %(task_id)s
)

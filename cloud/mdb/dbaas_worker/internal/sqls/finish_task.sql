SELECT code.finish_task(
    i_worker_id => %(worker_id)s,
    i_result    => %(result)s,
    i_changes   => %(changes)s,
    i_comment   => %(comment)s,
    i_task_id   => %(task_id)s,
    i_errors    => %(errors)s
)

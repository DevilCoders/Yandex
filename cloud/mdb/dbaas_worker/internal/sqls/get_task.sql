select awt.*
    from
            dbaas.worker_queue wq
        inner join
            dbaas.folders f
                on f.folder_id = wq.folder_id
        cross join lateral code.as_worker_task(wq.*, f.*) awt
    where
        wq.task_id = %(task_id)s;
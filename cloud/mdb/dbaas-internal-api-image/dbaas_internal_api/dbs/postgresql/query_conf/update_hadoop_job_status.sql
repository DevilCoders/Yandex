SELECT code.update_hadoop_job_status(
    i_job_id => %(job_id)s,
    i_status  => %(status)s,
    i_application_info  => %(application_info)s
)

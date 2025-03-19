SELECT job_id,
       cid,
       name,
       created_by,
       job_spec
FROM code.add_hadoop_job(
      i_cid        => %(cid)s,
      i_job_id     => %(job_id)s,
      i_name       => %(name)s,
      i_created_by => %(created_by)s,
      i_job_spec   => %(job_spec)s
)

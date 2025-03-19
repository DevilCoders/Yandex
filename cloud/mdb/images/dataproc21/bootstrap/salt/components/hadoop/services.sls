{% if not salt['ydputils.is_presetup']() %}

ydp-fs-available:
  {% if 'hdfs' == salt['ydp-fs.ydp_fs_protocol']() %}
    dataproc.hdfs_available:
    {% if salt['ydputils.is_masternode']() -%}
        - require:
            - service: service-hadoop-hdfs-namenode
    {% elif salt['ydputils.is_datanode']() -%}
        - require:
            - service: service-hadoop-hdfs-datanode
    {% else %}
        []
    {% endif %}
  {% elif 's3' == salt['ydp-fs.ydp_fs_protocol']() %}
    dataproc.s3_available
  {% elif salt['ydp-fs.is_singlenode']() %}
    test.succeed_without_changes:
        - name: "Can use local FS on single-node cluster"
  {% else %}
    test.fail_without_changes:
        - name: "Either enable HDFS component or provide S3 bucket for multi-node cluster shared FS"
  {% endif %}

{% endif %}

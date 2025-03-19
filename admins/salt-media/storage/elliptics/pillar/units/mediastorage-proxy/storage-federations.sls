mediastorage-mulcagate:
    use_file: true
    local_port: true
    run_tests: true
    port: 10011
    elliptics_cluster: true
    cfg_flags: 0
    threads: {{ (grains.get('num_cpus', 16) | int / 2) | int}}
    timeouts:
      read: 15
    timeout_coefs:
      data_flow_rate: 5
      for_commit: 5
    chunk_size:
      write: 20
      read: 25
    disable_auth: true
    elliptics_local_node: true
    die_limit: 0
    ping_stid: "2739.yadisk:uploader.97099719030826319143612335895"

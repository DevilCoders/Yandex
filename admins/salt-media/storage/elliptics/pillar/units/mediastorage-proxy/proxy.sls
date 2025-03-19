mediastorage-mulcagate:
    use_file: false
    local_port: true
    port: 10011
    run_tests: true
    elliptics_cluster: true
    threads: {{ (grains.get('num_cpus', 16) | int / 2) | int}}
    timeouts:
      read: 15
    timeout_coefs:
      data_flow_rate: 5
      for_commit: 5
    chunk_size:
      write: 20
      read: 25
    cache:
      enable: true

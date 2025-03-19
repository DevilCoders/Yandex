billing:
  solomon:
    cluster: hwci
  id_prefix: cb3
  api:
    tvm_enabled: False
  uploader:
    enabled: True
    parallel: True
    source:
      logbroker:
        auth:
          enabled: True
          client_id: "2000508"
        topics:
          - name: yc-test--billing-compute-instance
          - name: yc-test--billing-compute-snapshot
          - name: yc-test--billing-compute-image
          - name: yc-test--billing-object-storage
          - name: yc-test--billing-object-requests
          - name: yc-test--billing-nbs-volume
          - name: yc-test--billing-sdn-traffic
          - name: yc-test--billing-sdn-fip
          - name: yc-test--billing-ai-requests
  engine:
    logbroker:
      topics:
        - rt3.vla--yc-test--billing-compute-instance
        - rt3.vla--yc-test--billing-compute-snapshot
        - rt3.vla--yc-test--billing-compute-image
        - rt3.vla--yc-test--billing-object-storage
        - rt3.vla--yc-test--billing-object-requests
        - rt3.vla--yc-test--billing-nbs-volume
        - rt3.vla--yc-test--billing-sdn-traffic
        - rt3.vla--yc-test--billing-sdn-fip
        - rt3.vla--yc-test--billing-ai-requests

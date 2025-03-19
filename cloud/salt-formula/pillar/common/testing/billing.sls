# FIXME(CLOUD-17254): Rework billing pillar for testing cluster
billing:
  solomon:
    cluster: predf
  id_prefix: bu2
  s3:
    private_api: https://storage-idm.private-api.cloud-preprod.yandex.net:1443
    url: https://storage.cloud-preprod.yandex.net
    endpoint_url: https://storage.cloud-preprod.yandex.net
    reports_bucket: reports-testing
  api:
    auth_enabled: True
    tvm_enabled: False
  uploader:
    enabled: True
    parallel: True
    source:
      logbroker:
        topics:
          - name: yc-df-pre--billing-compute-instance
          - name: yc-df-pre--billing-compute-snapshot
          - name: yc-df-pre--billing-compute-image
          - name: yc-df-pre--billing-object-storage
          - name: yc-df-pre--billing-object-requests
          - name: yc-df-pre--billing-nbs-volume
          - name: yc-df-pre--billing-sdn-traffic
          - name: yc-df-pre--billing-sdn-fip
          - name: yc-df-pre--billing-ai-requests
  engine:
    logbroker:
      topics:
        - rt3.vla--yc-df-pre--billing-compute-instance
        - rt3.vla--yc-df-pre--billing-compute-snapshot
        - rt3.vla--yc-df-pre--billing-compute-image
        - rt3.vla--yc-df-pre--billing-object-storage
        - rt3.vla--yc-df-pre--billing-object-requests
        - rt3.vla--yc-df-pre--billing-nbs-volume
        - rt3.vla--yc-df-pre--billing-sdn-traffic
        - rt3.vla--yc-df-pre--billing-sdn-fip
        - rt3.vla--yc-df-pre--billing-ai-requests

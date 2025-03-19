push_client:
  instances:
    billing:
      enabled: True
      files:
        - name: /var/lib/yc/compute-node/accounting/accounting.log
          log_type: billing-compute-instance
        - name: /var/lib/yc/network-billing-collector/accounting/accounting.log
          log_type: billing-sdn-traffic
        - name: /var/log/nbs-server/nbs-metering.log
          log_type: billing-nbs-volume

    sdn_antifraud:
      files:
        - name: /var/lib/yc/network-billing-collector/antifraud/antifraud.log
          log_type: antifraud-overlay-flows

    cloud_logs:
      enabled: True
      files:
        - name: /var/lib/yc/serverless-functions/*/var/log/yc-logs.log
          log_type: yc-logs

hardware_platform:
  - name: xeon-e5-2660
    gpu: False
    cpus:
      - "Intel(R) Xeon(R) CPU E5-2660 v4 @ 2.00GHz"
      - "Intel(R) Xeon(R) CPU E5-2660 0 @ 2.20GHz"
      - "Intel(R) Xeon(R) CPU E5-2650 v2 @ 2.60GHz"
      - "Intel(R) Xeon(R) CPU E5-2690 v4 @ 2.60GHz"
      - "Intel(R) Xeon(R) CPU E5-2667 v4 @ 3.20GHz"
      - "Intel(R) Xeon(R) CPU E5-2683 v4 @ 2.10GHz"
      - "Intel Core Processor (Haswell, no TSX)" # CloudVM
  - name: xeon-e5-2660-smt
    gpu: True
    cpus:
      - "Intel(R) Xeon(R) CPU E5-2660 v4 @ 2.00GHz"
  - name: xeon-gold-6230
    gpu: False
    cpus:
      - "Intel(R) Xeon(R) Gold 6230 CPU @ 2.10GHz"

gpu_hardware_platform: "nvidia-tesla-gv100gl-pcie-32G"

push_client:
  instances:
    billing:
      enabled: True
      files:
        - name: /var/lib/yc/compute/accounting/image.log
          log_type: billing-compute-image
        - name: /var/lib/yc/compute/accounting/snapshot.log
          log_type: billing-compute-snapshot
        - name: /var/lib/yc/compute/accounting/address.log
          log_type: billing-sdn-fip
          
    billing-nlb:
        files:
        - name: /var/lib/yc/compute/accounting/balancer.log
          log_type: billing-nlb-balancer

==================================
Y.Cloud Billing Formula
==================================


Sample pillars
==============

**Billing API**

.. code-block:: yaml

  billing:
   oauth_token: 12345

   api:
     use_tvm: True
     allowed_tvm_services:
        - "77721212"
        - "23323223"
   tvm:
     url: https://tvm-api.yandex.net
     client_id: 12345
     client_secret: 54321

   balance:
     url: http://greed-tm.paysys.yandex.ru:8002/xmlrpc
     managed_uid: 98700241
     service_id: 143
     firm_id: 123
     payment_product_id: 1234
     service_token: <token>
     default_operator_id: 45370199
     use_tvm: True
     balance_tvm_client_id: 777

**Billing Collector & Uploader**

.. code-block:: yaml

 billing:

  monitoring:
    push_client:
      enabled: True

  uploader:
    enabled: True
    kikimr:
      database_id: billing
    source:

      file:
        - path: /var/lib/yc/billing-collector
          pattern: *.tskv.*

      logbroker:
        dc: man
        host: http://logbroker.yandex.net:8999
        client_id: yandexcloud-billing-pollster
        topics:
          - name: yc-billing-collector-vm-log
            ident: yc-billing-collector
          - name: dbaas-billing
            ident: dbaas

  push_client:
    enabled: True
    host: man.logbroker.yandex.net
    ident: yc-billing-collector
    files:
      - name: /var/lib/yc/billing-collector/vm/vm.tskv
        log_type: yc-billing-collector-vm-log

**Required grains:**

.. code-block:: yaml

  kikimr_id: default
  stand_type: virtual
  load_balancer:
    endpoints:
      identity:
        host: localhost
        port: 4336
      billing_kikimr_grpc:
        host: localhost
        port: 2136


**Monitoring:**

.. code-block:: yaml

 billing:
  monitoring:
    push-client:
      enabled: true

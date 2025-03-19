api:
  id_prefix: dh3
  default_os_product_category: "dh300000000000000000"
  default_publisher_category: "dh300000000000000001"
  default_var_category: "dh300000000000000002"
  default_isv_category: "dh300000000000000003"
  cloud_api:  https://iaas.private-api.gpn.yandexcloud.net:8093
  iam_api: https://identity.private-api.gpn.yandexcloud.net:14338
  compute_api:  https://iaas.private-api.gpn.yandexcloud.net:8093/compute/external
  mkt_api: https://mkt.private-api.gpn.yandexcloud.net:8443/marketplace
  mkt_private_api: https://mkt.private-api.gpn.yandexcloud.net:8443/marketplace
  billing_api: https://billing.private-api.gpn.yandexcloud.net:16465
  storage_api: https://storage-idm.private-api.cloud.yandex.net:1443
  cloud_id: "bn3j1lb4ar9bstmg16jh"
  pending_img_folder_id: "bn3e8ho4bor52v38dreg"
  public_img_folder_id: "standard-images"
  access_service:
    url: "as.private-api.gpn.yandexcloud.net:14286"
  ydb_client_version: 2
  kikimr:
    host: mkt-dn-spb99-1.svc.gpn.yandexcloud.net:2135
    database: /private-gpn-1_global/mkt
    root: /private-gpn-1_global/mkt
    enable_logging: false
  s3:
    url: "https://storage.yandexcloud.net"
    default_bucket: "gpn-yc-marketplace-default-images"
    versions_bucket: "gpn-version-images"
    products_bucket: "gpn-products"
    publishers_bucket: "gpn-publishers"
    eulas_bucket: "gpn-eulas"
  mds:
    internal_host: "avatars-int.mds.yandex.net:13000"
    host: "https://avatars.mds.yandex.net"
  logbroker:
    host: logbroker.yandex.net
    port: 2135
    topic_template: "yc-gpn/marketplace/export-{subject}"
    auth:
      destination: 2001059
  logbroker_export:
    tables:
      - "os_product"
      - "os_product_family"
      - "os_product_family_version"
      - "i18n"
  publisher_notification_email: yc-mkt-test@yandex-team.ru
  cache: "off"
  billing:
    sku:
      service_id: a6q18hf9bmtibm3ev42v
      balance_product_id: "509071"

api:
  id_prefix: dqn
  default_os_product_category: "dqn00000000000000000"
  default_publisher_category: "d7600000000000000001"
  default_var_category: "d7600000000000000002"
  default_isv_category: "d7600000000000000003"
  cloud_api: https://iaas.private-api.cloud-preprod.yandex.net
  iam_api: https://identity.private-api.cloud-preprod.yandex.net:14336
  compute_api: https://iaas.private-api.cloud-preprod.yandex.net/compute/external
  mkt_api: https://mkt.private-api.cloud-preprod.yandex.net:8443/marketplace
  mkt_private_api: https://mkt.private-api.cloud-preprod.yandex.net:8443/marketplace
  billing_api: https://billing.private-api.cloud-preprod.yandex.net:16465
  storage_api: https://storage-idm.private-api.cloud-preprod.yandex.net:1443
  cloud_id: "aoeaql9r10cd9cfue7v6"
  pending_img_folder_id: "aoeffnek5hctluaarncg"
  public_img_folder_id: "standard-images"
  access_service:
    url: "as.private-api.cloud-preprod.yandex.net:4286"
  ydb_client_version: 2
  kikimr:
    host: "grpcs://mkt-dn.ydb.cloud-preprod.yandex.net:2136"
    database: /pre-prod_global/mkt
    root: /pre-prod_global/mkt
    ydb_token_from_metadata: default-url
    root_ssl_cert_file: /etc/ssl/certs/ca-certificates.crt
    enable_logging: false
  s3:
    url: "https://storage.cloud-preprod.yandex.net"
    default_bucket: "yc-marketplace-default-images"
    versions_bucket: "version-images"
    products_bucket: "products"
    publishers_bucket: "publishers"
    eulas_bucket: "eulas"
  mds:
    internal_host: "avatars-int.mds.yandex.net:13000"
    host: "https://avatars.mds.yandex.net"
  service_account:
    key_id: "bfb42000000000000007"
    cloud_id: "aoe0000service0cloud"
    folder_id: "aoe00000000000000002"
    service_account_id: "bfb40000000000000007"
    service_account_login: "marketplace"
  logbroker:
    host: logbroker.yandex.net
    port: 2135
    topic_template: "yc-pre/marketplace/export-{subject}"
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

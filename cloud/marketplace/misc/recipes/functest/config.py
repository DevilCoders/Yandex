import os


COMMON_CFG = """
  identity:
    url: "https://identity.private-api.cloud-preprod.yandex.net:14336"
  kikimr:
    marketplace:
      host: localhost:2135
      database: /local
      root: /local
      enable_logging: True
marketplace:
  id_prefix: d76
  ordering_step: 10
  default_os_product_category: "d7600000000000000000"
  default_saas_product_category: "d7610000000000000000"
  default_simple_product_category: "d7620000000000000000"
  default_publisher_category: "d7600000000000000001"
  default_var_category: "d7600000000000000002"
  default_isv_category: "d7600000000000000003"
  service_id: "d7618hf9bmtibm3ev42v"
  balance_product_id: "509071"
dev: True
health:
    check_interval: 0
"""


PREFIX = os.getenv("TEST_WORK_PATH", "/tmp")


class Config:
    API_CONFIG_PATH = PREFIX + "/yc-marketplace-api.yaml"
    TEST_CONFIG_PATH = PREFIX + "/yc-marketplace-tests.yaml"

    @staticmethod
    def _test_cfg(api_port, as_http):
        with open(Config.TEST_CONFIG_PATH, "w") as f:
            f.writelines("""
endpoints:
  access_service:
    enabled: true
    url: "http://localhost:{as_http}"
    tls: false
  marketplace:
    url: "http://localhost:{api_port}/marketplace/v1alpha1/console"
  marketplace_private:
    url: "http://localhost:{api_port}/marketplace/v1alpha1/private"
{common}
""".format(api_port=api_port, as_http=as_http, common=COMMON_CFG))

    @staticmethod
    def _api_cfg(as_grpc):
        with open(Config.API_CONFIG_PATH, "w") as f:
            f.writelines("""
endpoints:
  access_service:
    enabled: true
    url: "localhost:{as_grpc}"
    tls: false
  s3:
    url: "https://storage.cloud-preprod.yandex.net"
    default_bucket: "yc-marketplace-dev-null"
    versions_bucket: "yc-marketplace-dev-null-1"
    products_bucket: "yc-marketplace-dev-null-1"
    publishers_bucket: "yc-marketplace-dev-null-1"
{common}
""".format(as_grpc=as_grpc, common=COMMON_CFG))

    @staticmethod
    def generate(api_port, as_grpc, as_http):
        Config._test_cfg(api_port=api_port, as_http=as_http)
        Config._api_cfg(as_grpc=as_grpc)

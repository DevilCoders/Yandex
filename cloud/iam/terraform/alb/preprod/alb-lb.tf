module "application_load_balancers" {
  source = "../modules/alb/v1"

  name                  = "auth-l7" # имя самого балансера
  folder_id             = local.openid_folder.id
  # адрес созданный ранее
  # 2a0d:d6c0:0:ff1c::3a0
  external_ipv6_addresses = [ for a in ycp_vpc_address.auth-l7.ipv6_address : a.address ]
  certificate_ids       = [
    "fd36047seu04g8r0dgqv",
  ]
  https_router_id        = ycp_platform_alb_http_router.default.id
  is_edge               = true
  sni_handlers          = [
    {
      name            = "auth-testing"
      server_names    = [
        "auth-testing.cloud.yandex.ru",
        "auth-testing.cloud.yandex.com",
      ]
      certificate_ids = [
        "fd36047seu04g8r0dgqv",
      ]
      http_router_id  = ycp_platform_alb_http_router.auth-router-testing["https"].id
      is_edge         = true
    }
  ]
  allocation            = {
    region_id  = "ru-central1"
    network_id = ycp_vpc_network.cloud-iam-preprod-nets.id
    locations  = [
      {
        subnet_id = ycp_vpc_subnet.cloud-iam-preprod-nets-ru-central1-a.id
        zone_id   = "ru-central1-a"
      },
      {
        subnet_id = ycp_vpc_subnet.cloud-iam-preprod-nets-ru-central1-b.id
        zone_id   = "ru-central1-b"
      },
      {
        subnet_id = ycp_vpc_subnet.cloud-iam-preprod-nets-ru-central1-c.id
        zone_id   = "ru-central1-c"
      },
    ]
  }
  solomon_cluster_name  = "cloud_preprod_auth-router"
  juggler_host          = "cloud_preprod_l7-auth-router"
}

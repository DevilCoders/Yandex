module "application_load_balancers" {
  source = "../modules/alb/v1"

  # id = ds7uj6g50oupsvqi9lh4
  name                  = "iam-ya-l7" # имя самого балансера
  folder_id             = local.iam_ya_prod_folder.id
  endpoint_ports        = [
    443, # all services
    4282, # token-service primary
    4286, # access-service primary
  ]
  # адрес созданный ранее
  # 2a0d:d6c0:0:1c::141
  external_ipv6_addresses = [ for a in ycp_vpc_address.iam-ya-l7.ipv6_address : a.address ]
  certificate_ids       = [
    "fpqvccnlfrnf7k2glj49", # *.cloud.yandex-team.ru
  ]
  https_router_id       = ycp_platform_alb_http_router.default.id
  is_edge               = false
  sni_handlers          = [
    {
      name            = "idm-ti-cloud-yandex-team-ru"
      server_names    = [
        "idm.ti.cloud.yandex-team.ru",
      ]
      certificate_ids = [
        "fpq340df637golim9mvf", # idm.ti.cloud.yandex-team.ru
      ]
      http_router_id  = ycp_platform_alb_http_router.default.id
      is_edge         = false
    }
  ]
  allocation            = {
    region_id  = "ru-central1"
    network_id = ycp_vpc_network.cloud-iam-ya-prod-nets.id
    locations  = [
      {
        subnet_id = ycp_vpc_subnet.cloud-iam-ya-prod-nets-ru-central1-a.id
        zone_id   = "ru-central1-a"
      },
      {
        subnet_id = ycp_vpc_subnet.cloud-iam-ya-prod-nets-ru-central1-b.id
        zone_id   = "ru-central1-b"
      },
      {
        subnet_id = ycp_vpc_subnet.cloud-iam-ya-prod-nets-ru-central1-c.id
        zone_id   = "ru-central1-c"
      },
    ]
  }
  solomon_cluster_name  = "cloud_prod_iam-ya-router"
  juggler_host          = "cloud_prod_iam-ya-l7-router"
}

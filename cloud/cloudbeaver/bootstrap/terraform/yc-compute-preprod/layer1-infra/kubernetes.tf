locals {
  cluster_name = "cloudbeaver"
}

module "k8s" {
  source = "../../modules/k8s/v1"

  healthchecks_cidrs = {
    v4 = local.v4_healthchecks_cidrs,
    v6 = local.v6_healthchecks_cidrs.preprod
  }
  yandex_nets            = local.yandexnets
  service_ipv4_range     = "10.96.0.0/16"
  cluster_ipv4_range     = "10.112.0.0/16"
  cluster_ipv6_range     = "fc00::/96"
  service_ipv6_range     = "fc01::/112"
  security_groups_ids    = [yandex_vpc_security_group.cloudbeaver-sg.id]
  cluster_name           = local.cluster_name
  folder_id              = var.folder_id
  network_id             = data.ycp_vpc_network.cloudbeaver.id
  locations              = local.k8s_locations
  registry_id            = yandex_container_registry.cloudbeaver.id
  docker_registry_pusher = var.bootstrap_user_id

  k8s_node_group = {
    cores         = 2
    core_fraction = 100
    memory        = 4
    size          = 2
  }

  providers = {
    yandex = yandex
  }
}

data "yandex_client_config" "client" {}

provider "kubernetes" {
  alias                  = "kubernetes"
  host                   = module.k8s.endpoint
  cluster_ca_certificate = module.k8s.ca
  token                  = data.yandex_client_config.client.iam_token
}

provider "kubectl" {
  host                   = module.k8s.endpoint
  cluster_ca_certificate = module.k8s.ca
  token                  = data.yandex_client_config.client.iam_token
  load_config_file       = false
}

resource "yandex_container_registry" "cloudbeaver" {
  name = "cloudbeaver"
  labels = {
    plane = "beaver"
  }
}

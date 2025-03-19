locals {
  collector_direct_dns_prefix = "jaeger-collector-direct-prod-{instance.internal_dc}{instance.index_in_zone}"
  collector_lb_dns_prefix     = "jaeger-collector-lb-prod-{instance.internal_dc}{instance.index_in_zone}"
  query_ydb2_dns_prefix       = "jaeger-query-ydb2-prod-{instance.internal_dc}{instance.index_in_zone}"
  reader_ydb2_dns_prefix      = "jaeger-lb-reader-ydb2-prod-{instance.internal_dc}{instance.index_in_zone}"

  zones = {
    vla = "ru-central1-a"
    sas = "ru-central1-b"
    myt = "ru-central1-c"
  }
  folder_id = "b1g74lha4qqoscuqktpo"
}

// TODO(zverre): use modules to share this parameters.
locals {
  subnets = [
    "e9b9e47n23i7a9a6iha7",
    "e2lt4ehf8hf49v67ubot",
    "b0c7crr1buiddqjmuhn7",
  ]
  ig_sa       = "aje90ppqftf2psmfdmoc"
  instance_sa = "aje9sbsqu8lh2f6ba3gh"
  image_id    = "fd8n055uqusbda25s7m1"
  environment = "prod"
}

provider "yandex" {
  endpoint  = "api.cloud.yandex.net:443"
  folder_id = local.folder_id
  token     = module.yc_token.result
}

provider "ycp" {
  prod      = true
  token     = module.yc_token.result
  folder_id = local.folder_id
}

data "template_file" "docker_json" {
  template = file("${path.module}/files/docker-json.tpl")

  vars = {
    docker_auth = module.docker_auth.value
  }
}

data "template_file" "empty_configs" {
  template = file("${path.module}/files/configs.tpl")

  vars = {
  }
}

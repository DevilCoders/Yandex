locals {
  collector_direct_dns_prefix = "jaeger-collector-direct-preprod-{instance.internal_dc}{instance.index_in_zone}"
  collector_lb_dns_prefix     = "jaeger-collector-lb-preprod-{instance.internal_dc}{instance.index_in_zone}"
  query_dns_prefix2           = "jaeger-query2-preprod-{instance.internal_dc}{instance.index_in_zone}"
  reader_global_dns_prefix2   = "jaeger-lb-reader-global2-preprod-{instance.internal_dc}{instance.index_in_zone}"

  zones = {
    vla = "ru-central1-a"
    sas = "ru-central1-b"
    myt = "ru-central1-c"
  }
  folder_id = "aoe6vkh56uk8rmulco19"
  image_id  = "fdvhniovafd6fefie9g0"
}

provider "yandex" {
  endpoint  = "api.cloud-preprod.yandex.net:443"
  folder_id = local.folder_id
  // jaeger
  zone = "ru-central1-c"
}

provider "ycp" {
  prod      = false
  token     = module.yc_token.result
  folder_id = local.folder_id
}

data "template_file" "docker_json" {
  template = file("${path.module}/files/docker-json.tpl")

  vars = {
    docker_auth = module.docker_auth.value
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/configs.tpl")

  vars = {
  }
}

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "secret" {
  description = "Secret where keys are stored"
}

// registry.yandex.net auth token
module "yav_secret_robot_yc_ai_docker_registry_yandex_net_auth" {
  source     = "../../modules/yav_secret"
  yt_oauth   = var.yandex_token
  id         = var.secret
  value_name = "docker-registry-yandex-net-auth"
}

// cr.yandex.net auth token
module "yav_secret_robot_yc_ai_docker_cr_yandex_auth" {
  source     = "../../modules/yav_secret"
  yt_oauth   = var.yandex_token
  id         = var.secret
  value_name = "docker-cr-yandex-auth"
}

data "template_file" "docker_json" {
  template = file("${path.module}/files/docker.tpl.json")

  vars = {
    docker_registry_yandex_net_auth = module.yav_secret_robot_yc_ai_docker_registry_yandex_net_auth.secret
    docker_cr_yandex_auth = module.yav_secret_robot_yc_ai_docker_cr_yandex_auth.secret
  }
}

output "rendered" {
  value = data.template_file.docker_json.rendered
}

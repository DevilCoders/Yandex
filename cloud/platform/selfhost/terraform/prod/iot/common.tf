provider "yandex" {
  endpoint  = var.yc_endpoint
  token     = var.yc_token
  folder_id = var.yc_folder
  zone      = var.yc_zone
}

module "ssh-keys" {
  source       = "../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "ycmqtt"
}

data "template_file" "docker_json" {
  template = file("${path.module}/files/docker.json.tpl")

  vars = {
    docker_auth = local.docker_auth
  }
}

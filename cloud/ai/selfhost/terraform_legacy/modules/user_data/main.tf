variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "extra_bootcmd" {
    description = "List of extra commands to execute durind boot"
    type        = list(list(string))
    default     = []
}

variable "extra_runcmd" {
  description = "List of extra commands to execute durind first start"
  type        = list(list(string))
  default     = []
}

variable "extra_user_data" {
  description = "Additional user data"
  default     = {
    mounts = []
  }
}

variable "abc_service" {
  description = "ABC service to populate users from"
  default     = "yc_ml_services"
}

module "user_ssh_keys" {
  source       = "../../modules/user_ssh_keys"
  yandex_token = var.yandex_token
  abc_service  = var.abc_service
}

data "template_file" "user_data" {
  template = file("${path.module}/files/user-data.tpl.yml")

  vars = {
    users = module.user_ssh_keys.users
    bootcmd = indent(2, yamlencode(
        concat(
            [
                [ "bash", "-c", "curl http://169.254.169.254/computeMetadata/v1/instance/hostname -H \"Metadata-Flavor:Google\" | sudo tee /etc/hostname | sudo xargs hostname" ],
                [ "sudo", "bash", "-c", "ip route get 8.8.8.8 | awk '{print $NF; exit}' | cut -d\".\" -f1-3 | xargs printf 'forward-zone:\n    name: \"internal.\"\n    forward-addr: %s.2\n' > /etc/unbound/unbound.conf.d/56-cloud-forward.conf"],
                [ "sudo", "bash", "-c", "ip route get 8.8.8.8 | awk '{print $NF; exit}' | cut -d\".\" -f1-3 | xargs printf 'forward-zone:\n    name: \"mdb.yandexcloud.net.\"\n    forward-addr: %s.2\n' >> /etc/unbound/unbound.conf.d/56-cloud-forward.conf"]
            ],
            var.extra_bootcmd
        )
    ))
    runcmd = indent(2, yamlencode(
        concat(
            [
                [ "bash", "-c", "echo \"First start, insert your initial command here\""],
            ],
            var.extra_runcmd
        )
    ))
    extra_user_data = indent(0, yamlencode(
        var.extra_user_data
    ))
  }
}

output "rendered" {
  value = data.template_file.user_data.rendered
}

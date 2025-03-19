/*
 * TODO: Refactor this entire module
 *       look like ot error prone 
 */

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
          # TODO Does it required? The hostname should be set automatically to valid
          ["bash", "-c", "curl http://169.254.169.254/computeMetadata/v1/instance/hostname -H \"Metadata-Flavor:Google\" | sudo tee /etc/hostname | sudo xargs hostname"],

          # FIXME: Delete this when all targets will migrate from undound to Cloud DNS
          #[ "sudo", "bash", "-c", "ip route get 8.8.8.8 | awk '{print $NF; exit}' | cut -d\".\" -f1-3 | xargs printf 'forward-zone:\n    name: \"internal.\"\n    forward-addr: %s.2\n' > /etc/unbound/unbound.conf.d/56-cloud-forward.conf"],
          #[ "sudo", "bash", "-c", "ip route get 8.8.8.8 | awk '{print $NF; exit}' | cut -d\".\" -f1-3 | xargs printf 'forward-zone:\n    name: \"mdb.yandexcloud.net.\"\n    forward-addr: %s.2\n' >> /etc/unbound/unbound.conf.d/56-cloud-forward.conf"]
        ],
        var.extra_bootcmd
      )
    ))
    runcmd = indent(2, yamlencode(
      concat(
        [
          ["bash", "-c", "echo \"First start, insert your initial command here\""],
        ],
        var.extra_runcmd
      )
    ))

    # FIXME: this is the most tricky place
    extra_user_data = length(var.extra_user_data) != 0 ? indent(0, yamlencode(var.extra_user_data)) : ""
  }
}


locals {
  users = yamldecode(module.user_ssh_keys.users)
  user_data = {
    bootcmd = concat(
      [
        ["bash", "-c", "curl http://169.254.169.254/computeMetadata/v1/instance/hostname -H \"Metadata-Flavor:Google\" | sudo tee /etc/hostname | sudo xargs hostname"],
        # [ "sudo", "bash", "-c", "ip route get 8.8.8.8 | awk '{print $NF; exit}' | cut -d\".\" -f1-3 | xargs printf 'forward-zone:\n    name: \"internal.\"\n    forward-addr: %s.2\n' > /etc/unbound/unbound.conf.d/56-cloud-forward.conf"],
        # [ "sudo", "bash", "-c", "ip route get 8.8.8.8 | awk '{print $NF; exit}' | cut -d\".\" -f1-3 | xargs printf 'forward-zone:\n    name: \"mdb.yandexcloud.net.\"\n    forward-addr: %s.2\n' >> /etc/unbound/unbound.conf.d/56-cloud-forward.conf"]
      ],
      var.extra_bootcmd
    )
    runcmd = concat(
      [
        ["bash", "-c", "echo \"First start, insert your initial command here\""],
      ],
      var.extra_runcmd
    )
    users = local.users
  }
}


variable "yandex_token" {
  type      = string
  sensitive = true
}
variable "checks" {}
variable "deploy_mark" {}

locals {
  checks_json = jsonencode(var.checks)
}

data "external" "write_checks" {
  program = ["${path.module}/dump_check.sh"]

  query = {
    mark   = var.deploy_mark
    checks = local.checks_json
  }
}

resource "null_resource" "apply_checks" {
  triggers = {
    checks = local.checks_json
    mark   = var.deploy_mark
  }

  provisioner "local-exec" {
    environment = {
      JUGGLER_OAUTH_TOKEN = "${var.yandex_token}"
    }
    command = "${path.module}/apply_checks.sh ${var.deploy_mark} '${var.yandex_token}'"
  }
}

output "checks_json" {
  value = local.checks_json
}

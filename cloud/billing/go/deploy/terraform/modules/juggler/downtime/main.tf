variable "fqdn" {}
variable "instance_id" {}


resource "null_resource" "downtime" {
  triggers = {
    fqdn        = var.fqdn
    instance_id = var.instance_id
  }

  provisioner "local-exec" {
    command = "${path.module}/apply_downtime.sh ${self.triggers.fqdn} 'downtime for ${self.triggers.fqdn} because of instance ${self.triggers.instance_id} destroy.'"
    when    = destroy
  }
}

output "downtime" {
  value = {
    fqdn : var.fqdn
  }
}

variable "files" {
  description = "files list"
}

data "template_file" "push_client_config" {
  template = file("${path.module}/files/push-client.tpl.yml")

  vars = {
    files         = indent(2, yamlencode(var.files))
  }
}

output "rendered" {
  value = data.template_file.push_client_config.rendered
}

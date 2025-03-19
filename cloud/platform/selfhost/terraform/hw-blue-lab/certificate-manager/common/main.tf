data "terraform_remote_state" "bootstrap-resources" {
  backend = "http"

  config = {
    address = "https://s3.mds.yandex.net/yc-bootstrap/terraform/hw-blue-lab/certificate-manager"
  }
}

data "template_file" "cloud-init-sh" {
  template = file("${path.module}/files/cloud-init.tpl.sh")

  vars = {
    local_lb_fqdn = var.local_lb_fqdn
    local_lb_addr = var.local_lb_addr
  }
}

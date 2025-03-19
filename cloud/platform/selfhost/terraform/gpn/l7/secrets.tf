locals {
  client_cert_hosts = "common-xds-client.gpn.yandexcloud.net"

  solomon_token_secret = "TODO"
}

resource "null_resource" "solomon_token" {
  triggers = {
    version = local.solomon_token_secret
  }

  provisioner "local-exec" {
    command = "echo dummy | ./kms_encrypt.sh > common/solomon-token.json"
  }
}

resource "null_resource" "client_cert" {
  triggers = {
    hosts = local.client_cert_hosts
  }

  provisioner "local-exec" {
    command = "./issue_cert.sh gpn_ca '${local.client_cert_hosts}' '${var.crt_token}' common/client"
  }
}

resource "null_resource" "cpl_cert" {
  triggers = {
    hosts = local.cpl.hosts
  }

  provisioner "local-exec" {
    command = "./issue_cert.sh gpn_ca '${local.cpl.hosts}' '${var.crt_token}' cpl/server"
  }
}

resource "null_resource" "api_cert" {
  triggers = {
    hosts = local.api.hosts
  }

  provisioner "local-exec" {
    command = "./issue_cert.sh gpn_ca '${local.api.hosts}' '${var.crt_token}' api/server"
  }
}

resource "null_resource" "api_cert_2" {
  triggers = {
    hosts = local.api.hosts_2
  }

  provisioner "local-exec" {
    command = "./issue_cert.sh gpn_int_ca '${local.api.hosts_2}' '${var.crt_token}' api/server-2"
  }
}

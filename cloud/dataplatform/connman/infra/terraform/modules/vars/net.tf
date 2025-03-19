locals {
  net = {
    "preprod" : local.net_preprod
    "prod" : local.net_prod
  }
  v4_cidr_blocks = {
    "ru-central1-a" : "172.16.0.0/16"
    "ru-central1-b" : "172.17.0.0/16"
    "ru-central1-c" : "172.18.0.0/16"
  }
  net_name           = "cloud-cm-${var.workspace}-nets"
  subnet_name_prefix = "cloud-cm-${var.workspace}-nets"
  net_preprod = {
    net_name : local.net_name,
    subnet_name_prefix : local.subnet_name_prefix,
    v4_cidr_blocks : local.v4_cidr_blocks,
    v6_cidr_blocks : {
      "ru-central1-a" : "2a02:6b8:c0e:501:0:fc7c:448f:0/112",
      "ru-central1-b" : "2a02:6b8:c02:901:0:fc7c:448f:0/112",
      "ru-central1-c" : "2a02:6b8:c03:501:0:fc7c:448f:0/112"
    }
  }
  net_prod = {
    net_name : local.net_name,
    subnet_name_prefix : local.subnet_name_prefix,
    v4_cidr_blocks : local.v4_cidr_blocks,
    v6_cidr_blocks : {
      "ru-central1-a" : "2a02:6b8:c0e:500:0:f899:3d0a:0/112",
      "ru-central1-b" : "2a02:6b8:c02:900:0:f899:3d0a:0/112",
      "ru-central1-c" : "2a02:6b8:c03:500:0:f899:3d0a:0/112"
    }
  }
}

# https://wiki.yandex-team.ru/users/e-sidorov/create-cloud-procedure/#posledovatelnostshagov

locals {
    #folder_id = "yc.iam.openid-server-folder"
    folder_id = "yc.iam.service-folder"
}

resource "ycp_vpc_network" "cloud-iam-preprod-nets" {
  lifecycle {
    prevent_destroy = true
  }

  name = "cloud-iam-preprod-nets"
  folder_id = local.folder_id
}

resource "ycp_vpc_subnet" "cloud-iam-preprod-nets-ru-central1-a" {
  lifecycle {
    prevent_destroy = true
  }

  # id = "buc5no3dassk3rf4q6os"
  v4_cidr_blocks = ["172.16.0.0/16"]
  v6_cidr_blocks = ["2a02:6b8:c0e:501:0:fc62::/112"] # _CLOUD_IAM_PREPROD_NETS_
  folder_id  = local.folder_id
  name       = "cloud-iam-preprod-nets-ru-central1-a"
  network_id = ycp_vpc_network.cloud-iam-preprod-nets.id
  zone_id    = "ru-central1-a"
  extra_params {
    export_rts = [
      "65533:666",
    ]
    feature_flags = [
      "hardened-public-ip",
      "blackhole",
    ]
    hbf_enabled = true
    import_rts = [
      "65533:776",
    ]
    rpf_enabled = false
  }
}

resource "ycp_vpc_subnet" "cloud-iam-preprod-nets-ru-central1-b" {
  lifecycle {
    prevent_destroy = true
  }

  # id = "bltmmrpmotdi740ktdsv"
  v4_cidr_blocks = ["172.17.0.0/16"]
  v6_cidr_blocks = ["2a02:6b8:c02:901:0:fc62::/112"] # _CLOUD_IAM_PREPROD_NETS_
  folder_id  = local.folder_id
  name       = "cloud-iam-preprod-nets-ru-central1-b"
  network_id = ycp_vpc_network.cloud-iam-preprod-nets.id
  zone_id    = "ru-central1-b"
  extra_params {
    export_rts = [
      "65533:666",
    ]
    feature_flags = [
      "hardened-public-ip",
      "blackhole",
    ]
    hbf_enabled = true
    import_rts = [
      "65533:776",
    ]
    rpf_enabled = false
  }
}

resource "ycp_vpc_subnet" "cloud-iam-preprod-nets-ru-central1-c" {
  lifecycle {
    prevent_destroy = true
  }

  # id = "fo2g1mg708d2du2mf67s"
  v4_cidr_blocks = ["172.18.0.0/16"]
  v6_cidr_blocks = ["2a02:6b8:c03:501:0:fc62::/112"] # _CLOUD_IAM_PREPROD_NETS_
  folder_id  = local.folder_id
  name       = "cloud-iam-preprod-nets-ru-central1-c"
  network_id = ycp_vpc_network.cloud-iam-preprod-nets.id
  zone_id    = "ru-central1-c"
  extra_params {
    export_rts = [
      "65533:666",
    ]
    feature_flags = [
      "hardened-public-ip",
      "blackhole",
    ]
    hbf_enabled = true
    import_rts = [
      "65533:776",
    ]
    rpf_enabled = false
  }
}

resource ycp_vpc_address auth-l7 {
  lifecycle {
    prevent_destroy = true # не даем удалять адрес чтобы не пришлось менять ДНС
  }

  # id = fo2facrgkqav950203ed

  folder_id = local.openid_folder.id
  name = "auth-l7-ipv6"

  ipv6_address_spec {
    requirements {
      hints = ["yandex-only"] # или ["yandex-only"] если надо адрес, доступный только из сетей яндекса.
    }
  }
  reserved = true # убирает лишний diff в terraform plan
}

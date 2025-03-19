# https://wiki.yandex-team.ru/users/e-sidorov/create-cloud-procedure/#posledovatelnostshagov

resource "ycp_vpc_network" "cloud-iam-internal-prestable-nets" {
    lifecycle {
        prevent_destroy = true
    }

    # id = "enpldtph8edblltiho0u"
    name = "cloud-iam-internal-prestable-nets"
    folder_id = local.iam_ya_prestable_folder.id
}

resource "ycp_vpc_subnet" "cloud-iam-internal-prestable-nets-ru-central1-a" {
    lifecycle {
        prevent_destroy = true
    }
    #id                = "e9burvj0plaalkp3p7ci"
    folder_id = local.iam_ya_prestable_folder.id
    name = "cloud-iam-internal-prestable-nets-ru-central1-a"
    network_id = ycp_vpc_network.cloud-iam-internal-prestable-nets.id
    v4_cidr_blocks = ["172.16.0.0/16"]
    v6_cidr_blocks = [
        "2a02:6b8:c0e:500:0:f860:c124:0/112",
    ] # _CLOUD_IAM_INTERNAL_PRESTABLE_NETS_
    zone_id = "ru-central1-a"
}

resource "ycp_vpc_subnet" "cloud-iam-internal-prestable-nets-ru-central1-b" {
    lifecycle {
        prevent_destroy = true
    }
    #id                = "e2lgpl24l4bm22b6auts"
    folder_id = local.iam_ya_prestable_folder.id
    name = "cloud-iam-internal-prestable-nets-ru-central1-b"
    network_id = ycp_vpc_network.cloud-iam-internal-prestable-nets.id
    v4_cidr_blocks = ["172.17.0.0/16"]
    v6_cidr_blocks = [
        "2a02:6b8:c02:900:0:f860:c124:0/112",
    ] # _CLOUD_IAM_INTERNAL_PRESTABLE_NETS_
    zone_id = "ru-central1-b"
}

resource "ycp_vpc_subnet" "cloud-iam-internal-prestable-nets-ru-central1-c" {
    lifecycle {
        prevent_destroy = true
    }
    #id                = "b0c8racqik675akbe0vs"
    folder_id = local.iam_ya_prestable_folder.id
    name = "cloud-iam-internal-prestable-nets-ru-central1-c"
    network_id = ycp_vpc_network.cloud-iam-internal-prestable-nets.id
    v4_cidr_blocks = ["172.18.0.0/16"]
    v6_cidr_blocks = [
        "2a02:6b8:c03:500:0:f860:c124:0/112",
    ] # _CLOUD_IAM_INTERNAL_PRESTABLE_NETS_
    zone_id = "ru-central1-c"
}

resource ycp_vpc_address iam-ya-prestable-l7 {
  lifecycle {
    prevent_destroy = true # не даем удалять адрес чтобы не пришлось менять ДНС
  }

  # id = b0c775i593pan715hk05

  folder_id = local.iam_ya_prestable_folder.id
  name = "iam-ya-prestable-l7-ipv6"

  ipv6_address_spec {
    requirements {
      hints = ["yandex-only"] # или ["yandex-only"] если надо адрес, доступный только из сетей яндекса.
    }
  }
  reserved = true # убирает лишний diff в terraform plan
}

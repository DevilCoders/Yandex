# https://wiki.yandex-team.ru/users/e-sidorov/create-cloud-procedure/#posledovatelnostshagov

resource "ycp_vpc_network" "cloud-iam-ya-prod-nets" {
    lifecycle {
        prevent_destroy = true
    }

    # id = "enph3rno6at97di4hoeb"
    name = "cloud-iam-ya-prod-nets"
    folder_id = local.iam_ya_prod_folder.id
}

resource "ycp_vpc_subnet" "cloud-iam-ya-prod-nets-ru-central1-a" {
    lifecycle {
        prevent_destroy = true
    }
    #id                = "e9bee4dcivv73ujueb9m"
    folder_id = local.iam_ya_prod_folder.id
    name = "cloud-iam-ya-prod-nets-ru-central1-a"
    network_id = ycp_vpc_network.cloud-iam-ya-prod-nets.id
    v4_cidr_blocks = ["172.16.0.0/16"]
    v6_cidr_blocks = [
        "2a02:6b8:c0e:500:0:f839:2f15:0/112",
    ] # _CLOUD_IAM_YA_PROD_NETS_
    zone_id = "ru-central1-a"
}

resource "ycp_vpc_subnet" "cloud-iam-ya-prod-nets-ru-central1-b" {
    lifecycle {
        prevent_destroy = true
    }
    #id                = "e2lie2vg1vl04evhut1o"
    folder_id = local.iam_ya_prod_folder.id
    name = "cloud-iam-ya-prod-nets-ru-central1-b"
    network_id = ycp_vpc_network.cloud-iam-ya-prod-nets.id
    v4_cidr_blocks = ["172.17.0.0/16"]
    v6_cidr_blocks = [
        "2a02:6b8:c02:900:0:f839:2f15:0/112",
    ] # _CLOUD_IAM_YA_PROD_NETS_
    zone_id = "ru-central1-b"
}

resource "ycp_vpc_subnet" "cloud-iam-ya-prod-nets-ru-central1-c" {
    lifecycle {
        prevent_destroy = true
    }
    #id                = "b0cv08p0lf9mfagk3r03"
    folder_id = local.iam_ya_prod_folder.id
    name = "cloud-iam-ya-prod-nets-ru-central1-c"
    network_id = ycp_vpc_network.cloud-iam-ya-prod-nets.id
    v4_cidr_blocks = ["172.18.0.0/16"]
    v6_cidr_blocks = [
        "2a02:6b8:c03:500:0:f839:2f15:0/112",
    ] # _CLOUD_IAM_YA_PROD_NETS_
    zone_id = "ru-central1-c"
}

resource ycp_vpc_address iam-ya-l7 {
  lifecycle {
    prevent_destroy = true # не даем удалять адрес чтобы не пришлось менять ДНС
  }

  # id = b0cdv38efioiqkkqag20

  folder_id = local.iam_ya_prod_folder.id
  name = "iam-ya-l7-ipv6"

  ipv6_address_spec {
    requirements {
      hints = ["yandex-only"] # или ["yandex-only"] если надо адрес, доступный только из сетей яндекса.
    }
  }
  reserved = true # убирает лишний diff в terraform plan
}

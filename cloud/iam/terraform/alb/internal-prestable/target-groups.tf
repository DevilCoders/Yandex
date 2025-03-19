locals {
    iam_internal_prestable_myt1_ip = "2a02:6b8:c03:500:0:f860:c124:2e5" # iam-internal-prestable-myt1.svc.cloud.yandex.net
    iam_internal_prestable_sas1_ip = "2a02:6b8:c02:900:0:f860:c124:1c" # iam-internal-prestable-sas1.svc.cloud.yandex.net
    iam_internal_prestable_vla1_ip = "2a02:6b8:c0e:500:0:f860:c124:6f" # iam-internal-prestable-vla1.svc.cloud.yandex.net
}

resource "ycp_platform_alb_target_group" iam_ya_prestable_target_group {
    # id = ds7fk4u56cjbvoivjunh
    description = "IAM yandex-team prestable target group for ALB"
    folder_id = local.iam_ya_prestable_folder.id
    labels = {}
    name = "tg-iam-internal-prestable"

    target {
        ip_address = local.iam_internal_prestable_myt1_ip
        subnet_id = ycp_vpc_subnet.cloud-iam-internal-prestable-nets-ru-central1-c.id
        locality {
            zone_id = ycp_vpc_subnet.cloud-iam-internal-prestable-nets-ru-central1-c.zone_id
        }
    }
    target {
        ip_address = local.iam_internal_prestable_sas1_ip
        subnet_id = ycp_vpc_subnet.cloud-iam-internal-prestable-nets-ru-central1-b.id
        locality {
            zone_id = ycp_vpc_subnet.cloud-iam-internal-prestable-nets-ru-central1-b.zone_id
        }
    }
    target {
        ip_address = local.iam_internal_prestable_vla1_ip
        subnet_id = ycp_vpc_subnet.cloud-iam-internal-prestable-nets-ru-central1-a.id
        locality {
            zone_id = ycp_vpc_subnet.cloud-iam-internal-prestable-nets-ru-central1-a.zone_id
        }
    }
}

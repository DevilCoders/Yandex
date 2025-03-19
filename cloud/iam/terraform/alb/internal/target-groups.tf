locals {
    iam_ya_myt1_ip = "2a02:6b8:c03:500:0:f839:2f15:2ea"  # iam-ya-myt1.svc.cloud.yandex.net
    iam_ya_sas1_ip = "2a02:6b8:c02:900:0:f839:2f15:3b7"  # iam-ya-sas1.svc.cloud.yandex.net
    iam_ya_vla1_ip = "2a02:6b8:c0e:500:0:f839:2f15:3db"  # iam-ya-vla1.svc.cloud.yandex.net
}

resource "ycp_platform_alb_target_group" iam_ya_target_group {
    # id = ds7kt53r72koj7ornivo
    description = "IAM yandex-team target group for ALB"
    folder_id   = local.iam_ya_prod_folder.id
    labels      = {}
    name        = "iam-ya-target-group"

    target {
        ip_address = local.iam_ya_myt1_ip
        subnet_id = ycp_vpc_subnet.cloud-iam-ya-prod-nets-ru-central1-c.id
        locality {
            zone_id = ycp_vpc_subnet.cloud-iam-ya-prod-nets-ru-central1-c.zone_id
        }
    }

    target {
        ip_address = local.iam_ya_sas1_ip
        subnet_id = ycp_vpc_subnet.cloud-iam-ya-prod-nets-ru-central1-b.id
        locality {
            zone_id = ycp_vpc_subnet.cloud-iam-ya-prod-nets-ru-central1-b.zone_id
        }
    }

    target {
        ip_address = local.iam_ya_vla1_ip
        subnet_id = ycp_vpc_subnet.cloud-iam-ya-prod-nets-ru-central1-a.id
        locality {
            zone_id = ycp_vpc_subnet.cloud-iam-ya-prod-nets-ru-central1-a.zone_id
        }
    }
}

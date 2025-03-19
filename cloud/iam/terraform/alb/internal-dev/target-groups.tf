locals {
    iam_internal_dev_vla1_ip = "2a02:6b8:c0e:500:0:f861:9eab:284" # iam-internal-dev-vla1.svc.cloud.yandex.net
    iam_internal_dev_sas1_ip = "2a02:6b8:c02:900:0:f861:9eab:2ab" # iam-internal-dev-sas1.svc.cloud.yandex.net
    iam_internal_dev_myt1_ip = "2a02:6b8:c03:500:0:f861:9eab:3fb" # iam-internal-dev-myt1.svc.cloud.yandex.net
}

resource "ycp_platform_alb_target_group" iam_ya_dev_target_group {
    # id = ds7i8eok9hqu65runebc
    description = "IAM yandex-team dev target group for ALB"
    folder_id = local.iam_ya_dev_folder.id
    labels = {}
    name = "tg-iam-internal-dev"
    
    target {
        ip_address = local.iam_internal_dev_vla1_ip
        subnet_id = ycp_vpc_subnet.cloud-iam-internal-dev-nets-ru-central1-a.id
        locality {
            zone_id = ycp_vpc_subnet.cloud-iam-internal-dev-nets-ru-central1-a.zone_id
        }
    }        
    
    target {
        ip_address = local.iam_internal_dev_sas1_ip
        subnet_id = ycp_vpc_subnet.cloud-iam-internal-dev-nets-ru-central1-b.id
        locality {
            zone_id = ycp_vpc_subnet.cloud-iam-internal-dev-nets-ru-central1-b.zone_id
        }
    }

    target {
        ip_address = local.iam_internal_dev_myt1_ip
        subnet_id = ycp_vpc_subnet.cloud-iam-internal-dev-nets-ru-central1-c.id
        locality {
            zone_id = ycp_vpc_subnet.cloud-iam-internal-dev-nets-ru-central1-c.zone_id
        }
    }
}

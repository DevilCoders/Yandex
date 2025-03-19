resource "ycp_platform_alb_target_group" iam_openid_target_group {
    description = "IAM target group for ALB"
    folder_id   = local.openid_folder.id
    labels      = {}
    name        = "iam-openid-target-group"
    # id = "ds70kei6ogf5aan66jgt"

    # iam-openid-myt1.svc.cloud.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:2206:9cbe:7ff:feae:b351"
    }
    # iam-openid-myt2.svc.cloud.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:200f:9cbe:5ff:fe55:f647"
    }
    # iam-openid-myt3.svc.cloud.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:220a:9cbe:cff:febf:8287"
    }
    # iam-openid-vla1.svc.cloud.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:10c:9cbe:cff:fe56:1d3"
    }
    # iam-openid-vla2.svc.cloud.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:b:4cf6:4ff:fe09:88b9"
    }
    # iam-openid-vla3.svc.cloud.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:12:e8ea:5ff:fec3:a70f"
    }
    # iam-openid-sas1.svc.cloud.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:102a:e8ea:bff:fec3:a687"
    }
    # iam-openid-sas2.svc.cloud.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:1007:9cbe:aff:feae:b345"
    }
    # iam-openid-sas3.svc.cloud.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:1007:9cbe:4ff:feae:bb91"
    }
}

resource "ycp_platform_alb_target_group" iam_openid_target_group_preprod {
    description = "IAM target group for ALB"
    folder_id   = local.openid_folder.id
    labels      = {}
    name        = "iam-openid-target-group-preprod"
    # id = "a5dp6l18tql8repjk6g1"

    # iam-openid-myt1.svc.cloud-preprod.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:2240:9cbe:4ff:febf:8297"
    }
    # iam-openid-vla1.svc.cloud-preprod.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:41:9cbe:aff:fea7:28e8"
    }
    # iam-openid-sas1.svc.cloud-preprod.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:1042:9cbe:22ff:febf:8b47"
    }
    # iam-openid-myt2.svc.cloud-preprod.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:2240:9cbe:4ff:febf:827f"
    }
    # iam-openid-vla2.svc.cloud-preprod.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:41:9cbe:5ff:fea7:27a0"
    }
    # iam-openid-sas2.svc.cloud-preprod.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:1042:9cbe:16ff:fea7:1fd9"
    }
}

resource "ycp_platform_alb_target_group" iam_openid_target_group_testing {
    description = "IAM OAuth/OpenID Server target group for ALB (testing)"
    folder_id   = "yc.iam.openid-server-folder"
    labels      = {}
    name        = "iam-openid-target-group-testing"
    # id = "a5diesp8ur6keoqqi5ar"

    # iam-openid-myt1.svc.cloud-testing.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:2300:4cf6:cff:fe09:9195"
    }
    # iam-openid-sas1.svc.cloud-testing.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:1300:8839:4ff:fec6:3499"
    }
    # iam-openid-vla1.svc.cloud-testing.yandex.net
    target {
        ip_address = "2a02:6b8:bf00:300:9cbe:9ff:febf:8ffe"
    }
}

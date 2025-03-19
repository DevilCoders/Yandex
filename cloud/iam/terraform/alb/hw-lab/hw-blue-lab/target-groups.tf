resource "ycp_platform_alb_target_group" "iam_openid_target_group" {
  description = "IAM target group for ALB"
  folder_id   = "yc.iam.openid-server-folder"
  labels      = {}
  name        = "iam-openid-target-group"
# TODO
#  # iam-openid-bla1.svc.hw-blue.cloud-lab.yandex.net
#  target {
#    ip_address = "2a02:6b8:bf00:2240:9cbe:5ff:febf:9837"
#  }
  # iam-openid-blb1.svc.hw-blue.cloud-lab.yandex.net
  target {
    ip_address = "2a02:6b8:bf00:341:e8c9:9ff:fed2:b060"
    external_address = true
#    locality {
#      zone_id = local.vpc.auth.subnets.b.zone_id
#    }
  }
# TODO
#  # iam-openid-blc1.svc.hw-blue.cloud-lab.yandex.net
#  target {
#    ip_address = "2a02:6b8:bf00:1042:9cbe:fff:fea7:2081"
#  }
}

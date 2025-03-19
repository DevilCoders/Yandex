resource "ycp_platform_alb_target_group" "iam_openid_target_group" {
  lifecycle {
    prevent_destroy = true
  }

  description = "IAM target group for ALB"
  folder_id   = "yc.iam.openid-server-folder"
  labels      = {}
  name        = "iam-openid-target-group"

  # iam-openid-il1-a1.svc.yandexcloud.co.il
  target {
    ip_address       = "2a11:f740:0:102:5cc1:14ff:fede:e97a"
    external_address = true
  }
  # iam-openid-il1-a2.svc.yandexcloud.co.il
  target {
    ip_address       = "2a11:f740:0:100:e8c9:1fff:feb8:2328"
    external_address = true
  }
  # iam-openid-il1-a3.svc.yandexcloud.co.il
  target {
    ip_address       = "2a11:f740:0:100:5cc1:8ff:fede:ecf8"
    external_address = true
  }
}

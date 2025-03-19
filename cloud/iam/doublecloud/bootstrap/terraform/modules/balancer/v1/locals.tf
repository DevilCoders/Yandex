locals {
  iam_public_alb_name = "iam-public-alb-${var.infra.name}"
  operations_alb_name = "iam-operations-${var.infra.name}"

  health_check_tcp = {
    protocol = "TCP"
    path     = null
    port     = null
    matcher  = null
  }

  service_ports_grpc = {
    as  = {
      service_name = "access-service"
      port         = 4286
      port_name    = "as-grpc"
    }
    iam = {
      service_name = "iam-control-plane"
      port         = 4283
      port_name    = "iam-grpc"
    }
    org = {
      service_name = "org-service"
      port         = 4290
      port_name    = "org-grpc"
    }
    rm  = {
      service_name = "rm-control-plane"
      port         = 4284
      port_name    = "rm-grpc"
    }
    ss  = {
      service_name = "openid-server"
      port         = 8655
      port_name    = "ss-grpc"
    }
    ts  = {
      service_name = "token-service"
      port         = 4282
      port_name    = "ts-grpc"
    }
  }

  service_ports_grpc_values = [for _, v in local.service_ports_grpc : v.port]

  service_ports_https = {
    identity = {
      service_name = "identity"
      port         = 14336
      port_name    = "identity-https"
    }
    oauth    = {
      service_name = "openid-server"
      port         = 9090
      port_name    = "oauth-https"
    }
  }

  service_ports_others = {
    auth_ui = {
      service_name = "auth-ui"
      port         = 80
      port_name    = "auth-ui-http"
    }
  }

  service_aliases = concat(
    keys(local.service_ports_grpc),
    keys(local.service_ports_https),
  )

  //  https://racktables.yandex.net/export/expand-trypo-macros.php?macro=_YANDEXNETS_
  yandexnets = {
    ipv4 = [
      "5.45.192.0/18",
      "5.255.192.0/18",
      "37.9.64.0/18",
      "37.140.128.0/18",
      "45.87.132.0/22",
      "77.88.0.0/18",
      "84.252.160.0/19",
      "87.250.224.0/19",
      "90.156.176.0/22",
      "93.158.128.0/18",
      "95.108.128.0/17",
      "100.43.64.0/19",
      "139.45.249.96/29", // https://st.yandex-team.ru/NOCREQUESTS-25161
      "141.8.128.0/18",
      "178.154.128.0/18",
      "185.32.187.0/24",
      "199.21.96.0/22",
      "199.36.240.0/22",
      "213.180.192.0/19",
    ]
    ipv6 = [
      "2620:10f:d000::/44",
      "2a02:6b8::/32",
    ]
  }
}

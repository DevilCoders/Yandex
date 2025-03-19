locals {
  iam_backends_defaults = {

    "as" = {
      is_http               = false
      name                  = "access-service"
      backend_port          = 4286
      healthcheck_port      = 4285
      http_healthcheck_path = "/ping"
    }

    "iam" = {
      is_http               = false
      name                  = "iam-control-plane"
      backend_port          = 4283
      healthcheck_port      = 9982
      http_healthcheck_path = "/ping"
    }

    "identity" = {
      is_http               = true
      name                  = "identity-private"
      backend_port          = 14336
      healthcheck_port      = null
      http_healthcheck_path = "/private/v1/ping"
    }

    "oauth" = {
      is_http               = true
      name                  = "openid-server"
      backend_port          = 9090
      healthcheck_port      = null
      http_healthcheck_path = "/ping"
    }

    "org" = {
      is_http               = false
      name                  = "org-service"
      backend_port          = 4290
      healthcheck_port      = 9984
      http_healthcheck_path = "/ping"
    }

    "reaper" = {
      is_http               = false
      name                  = "reaper"
      backend_port          = 7376
      healthcheck_port      = 7381
      http_healthcheck_path = "/ping"
    }

    "rm" = {
      is_http               = false
      name                  = "rm-control-plane"
      backend_port          = 4284
      healthcheck_port      = 9983
      http_healthcheck_path = "/ping"
    }

    "ss" = {
      is_http               = false
      name                  = "session-service"
      backend_port          = 8655
      healthcheck_port      = 9090
      http_healthcheck_path = "/ping"
    }

    "ti" = {
      is_http               = false
      name                  = "team-integration"
      backend_port          = 4687
      healthcheck_port      = 4367
      http_healthcheck_path = "/ping"
    }

    "ti-idm" = {
      is_http               = true
      name                  = "team-integration-idm"
      backend_port          = 8443
      healthcheck_port      = 4367
      http_healthcheck_path = "/ping"
    }

    "ts" = {
      is_http               = false
      name                  = "token-service"
      backend_port          = 4282
      healthcheck_port      = 9981
      http_healthcheck_path = "/ping"
    }
  }
}

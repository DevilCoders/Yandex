locals {
  cpl_image           = "fdvk6h1l739lu626rmj1"

  cpl_service_account = "bfbgm268434joruq0pfd"
}

module cpl {
    source = "../ig-ycp"

    name        = "mdbproxy-cpl"
    description = "MDBProxy-CPL IG"
    service     = "mdbproxy-cpl"

    amount = 1
    cores  = 2
    memory = 4
    core_fraction = 20
    platform_id="standard-v2"

    boot_disk_image_id = local.cpl_image

    folder_id                   = local.yc_folder
    ssh-keys                    = module.ssh-keys.ssh-keys
    conductor_group             = "serverless-mdbproxy-cpl"
    service_account_id          = local.yc_ig_sacc
    instance_service_account_id = local.cpl_service_account
    l7_tg                       = true
    subnets                     = local.subnets
    instance_metadata = merge(local.arbitrary_metadata, {
        attrs      = <<-EOF
    internal_dc
  EOF
        ssh-keys   = module.ssh-keys.ssh-keys
        mdbp_id_prefix = "akf"
        mdbp_kikimr_endpoint = "lb.cc8005t7det2gmkn05rr.ydb.mdb.cloud-preprod.yandex.net:2135"
        mdbp_kikimr_tablespace = "/pre-prod_global/aoepnsltlaqvs349v7h6/cc8005t7det2gmkn05rr/cpl"
        mdbp_kikimr_database = "/pre-prod_global/aoepnsltlaqvs349v7h6/cc8005t7det2gmkn05rr"
        mdbp_auth_endpoint = "as.private-api.cloud-preprod.yandex.net:4286"
        mdbp_resource_manager_endpoint = "api-adapter.private-api.ycp.cloud-preprod.yandex.net:443"
        mdbp_kms_crypto_endpoint = "kms.cloud-preprod.yandex.net:8443"
        mdbp_kms_encryption_key_id = "e104u3v8lv9gnha037i8"
        mdbp_postgre_endpoint = "api-adapter.private-api.ycp.cloud-preprod.yandex.net:443"
        mdbp_grpc_cert_file_path = "/etc/yc-mdbproxy/cpl/grpc-cert.pem"
        mdbp_postgre_dpl_shards_hosts = "postgre1.serverless.cloud-preprod.yandex.net"
        mdbp_postgre_refresh_shards_delay_sec="61"
        mdbp_folder_id = local.yc_folder

    })
}

resource "ycp_platform_alb_backend_group" mdbproxy_cpl_backend_group {
    description = "Backend for mdbproxy service (preprod)"
    name        = "mdbproxy-cpl-preprod-backend-group"

    grpc {
        backend {
            name   = "gprc-backend"
            weight = 100
            port   = 443
            target_group {
                target_group_id = module.cpl.group.platform_l7_load_balancer_state[0].target_group_id
            }

            healthchecks {
                healthy_threshold   = 1
                interval            = "1s"
                timeout             = "0.100s"
                unhealthy_threshold = 2
                grpc {
                  service_name = "yandex.cloud.priv.serverless.mdbproxy.v1.ProxyService"
                }
            }

            passive_healthcheck {
              max_ejection_percent = 66
              consecutive_gateway_failure = 2
              base_ejection_time = "30s"
              enforcing_consecutive_gateway_failure = 100
              interval = "10s"
            }

            tls {}
        }
        connection {}
    }
}

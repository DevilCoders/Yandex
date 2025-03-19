locals {
  fqdn_operations   = "ops.private-api.iam.internal.yadc.tech"
  fqdn_internal_iam = "preprod.iam.internal.yadc.tech"
  zone_internal_iam =         "iam.internal.yadc.tech"
  zone_private_iam  =             "internal.yadc.tech"
  zone_tech_iam     =                  "iam.yadc.tech"

  fqdn_public_auth  =                 "auth.yadc.io"
  zone_public_auth  =                 "auth.yadc.io"

  # TODO Replace it
  zone_private_legacy                 =           "internal.yadc.io"
  fqdn_notify_legacy                  = "notify.ui.internal.yadc.io"
  notify_endpoint_service_name_legacy = "com.amazonaws.vpce.eu-central-1.vpce-svc-05f661e21112ed000"

  billing_endpoint_fqdn           = "preprod.api.billing.internal.yadc.tech"
  billing_endpoint_service_name   = "com.amazonaws.vpce.eu-central-1.vpce-svc-0b39d57b6cf233f08"

  mdb_endpoint_fqdn_legacy         = "preprod.mdb.internal.yadc.io"
  mdb_endpoint_service_name_legacy = "com.amazonaws.vpce.eu-central-1.vpce-svc-0e245f909c1a02f41"

  vpc_endpoint_fqdn_legacy         = "vpc.preprod.mdb.internal.yadc.io"
  vpc_endpoint_service_name_legacy = "com.amazonaws.vpce.eu-central-1.vpce-svc-03a27e67e6eec075c"

  prometheus_reader_accounts = [
    "737497888847", // infraplane PROD
  ]
}

output "fqdn_operations" {
  value = local.fqdn_operations
}

output "fqdn_internal_iam" {
  value = local.fqdn_internal_iam
}

output "zone_internal_iam" {
  value = local.zone_internal_iam
}


output "zone_tech_iam" {
  value = local.zone_tech_iam
}

output "fqdn_public_auth" {
  value = local.fqdn_public_auth
}

output "zone_public_auth" {
  value = local.zone_public_auth
}

output "zone_private_iam" {
  value = local.zone_private_iam
}

# TODO Remove it
output "zone_private_legacy" {
  value = local.zone_private_legacy
}
# TODO Replace it
output "fqdn_notify" {
  value = local.fqdn_notify_legacy
}

output "notify_endpoint_service_name" {
  value = local.notify_endpoint_service_name_legacy
}

output "billing_endpoint_fqdn" {
  value = local.billing_endpoint_fqdn
}

output "billing_endpoint_service_name" {
  value = local.billing_endpoint_service_name
}

output "mdb_endpoint_fqdn" {
  value = local.mdb_endpoint_fqdn_legacy
}

output "mdb_endpoint_service_name" {
  value = local.mdb_endpoint_service_name_legacy
}

output "vpc_endpoint_fqdn" {
  value = local.vpc_endpoint_fqdn_legacy
}

output "vpc_endpoint_service_name" {
  value = local.vpc_endpoint_service_name_legacy
}

output "public_zones" {
  value = [
    local.zone_public_auth,
    local.zone_tech_iam,
    local.zone_internal_iam,
  ]
}

output "private_zones" {
  value = [
    local.zone_private_iam,
    # TODO Remove it
    local.zone_private_legacy,
  ]
}

output "prometheus_reader_accounts" {
  value = local.prometheus_reader_accounts
}

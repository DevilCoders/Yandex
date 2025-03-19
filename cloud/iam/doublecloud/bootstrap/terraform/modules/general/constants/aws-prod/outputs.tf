locals {
  fqdn_operations   = "ops.private-api.iam.internal.double.tech"
  fqdn_internal_iam = "prod.iam.internal.double.tech"
  zone_internal_iam =      "iam.internal.double.tech"
  zone_private_iam  =          "internal.double.tech"
  zone_tech_iam     =               "iam.double.tech"
  fqdn_public_auth  =              "auth.double.cloud"
  zone_public_auth  =              "auth.double.cloud"

  fqdn_notify                  = "notify.ui.internal.double.tech"
  notify_endpoint_service_name = "com.amazonaws.vpce.eu-central-1.vpce-svc-018dfdcf803ce8ea7"

  billing_endpoint_fqdn           = "prod.api.billing.internal.double.tech"
  billing_endpoint_service_name   = "com.amazonaws.vpce.eu-central-1.vpce-svc-08691e5a4476b4d6b"

  mdb_endpoint_fqdn_legacy         = "prod.mdb.internal.double.tech"
  mdb_endpoint_service_name_legacy = "com.amazonaws.vpce.eu-central-1.vpce-svc-02880e5ba6f19936a"

  vpc_endpoint_fqdn_legacy         = "vpc.prod.mdb.internal.double.tech"
  vpc_endpoint_service_name_legacy = "com.amazonaws.vpce.eu-central-1.vpce-svc-0507064fc99b7891a"

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

output "fqdn_notify" {
  value = local.fqdn_notify
}

output "notify_endpoint_service_name" {
  value = local.notify_endpoint_service_name
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
  ]
}

output "prometheus_reader_accounts" {
  value = local.prometheus_reader_accounts
}

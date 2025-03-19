locals {
  docker_auth = coalesce(
    var.docker_auth,
    module.yav-secret-docker_auth.value,
    "there-is-no-valid-value-detected!",
  )

  devices_cert = var.devices_cert_file != "_intentionally_empty_file" ? file(var.devices_cert_file) : module.yav-secret-config_devices_cert.value
  mqtt_cert    = var.mqtt_cert_file != "_intentionally_empty_file" ? file(var.mqtt_cert_file) : module.yav-secret-config_mqtt_cert.value
  mqtt_key     = var.mqtt_key_file != "_intentionally_empty_file" ? file(var.mqtt_key_file) : module.yav-secret-config_mqtt_key.value
  mqtt_self_ping_sub_cert    = module.yav-secret-config_mqtt_selfping_sub_cert.value
  mqtt_self_ping_pub_cert    = module.yav-secret-config_mqtt_selfping_pub_cert.value
}

module "yav-secret-docker_auth" {
  source    = "../../modules/yav"
  secret_id = "sec-01cx8a81rj3458rhqaj5x1sztb"
  key_id    = "auth_string"
}

module "yav-secret-config_devices_cert" {
  source    = "../../modules/yav"
  secret_id = "sec-01db2n5neq35xagaq636h63y5a"
  key_id    = "cert"
}

module "yav-secret-config_mqtt_cert" {
  source    = "../../modules/yav"
  secret_id = "sec-01db2njt48rp8ccz7hesz2g0er"
  key_id    = "cert"
}

module "yav-secret-config_mqtt_key" {
  source    = "../../modules/yav"
  secret_id = "sec-01db2njt48rp8ccz7hesz2g0er"
  key_id    = "key"
}

module "yav-secret-tvm_token" {
  source    = "../../modules/yav"
  secret_id = "sec-01db2mwska0sd3rz3mtvkgcskv"
  key_id    = "tvmtool_token"
}

module "yav-secret-tvm_secret" {
  source    = "../../modules/yav"
  secret_id = "sec-01db2mwska0sd3rz3mtvkgcskv"
  key_id    = "secret"
}

module "yav-secret-tvm_client_id" {
  source    = "../../modules/yav"
  secret_id = "sec-01db2mwska0sd3rz3mtvkgcskv"
  key_id    = "client_id"
}

module "yav-secret-events-tvm_token" {
  source    = "../../modules/yav"
  secret_id = "sec-01dmz7ef06vrd59c5wjchyr03z"
  key_id    = "tvmtool_token"
}

module "yav-secret-events-tvm_secret" {
  source    = "../../modules/yav"
  secret_id = "sec-01dmz7ef06vrd59c5wjchyr03z"
  key_id    = "secret"
}

module "yav-secret-events-tvm_client_id" {
  source    = "../../modules/yav"
  secret_id = "sec-01dmz7ef06vrd59c5wjchyr03z"
  key_id    = "client_id"
}

// selfping
module "yav-secret-config_mqtt_selfping_sub_cert" {
  source    = "../../modules/yav"
  secret_id = "sec-01dnmbgp471k76ayxv4bg2hmdr"
  key_id    = "sub-cert"
}

module "yav-secret-config_mqtt_selfping_pub_cert" {
  source    = "../../modules/yav"
  secret_id = "sec-01dnmbgp471k76ayxv4bg2hmdr"
  key_id    = "pub-cert"
}

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
  secret_id = "sec-01d61132ge8dnfagfwgnwx17hv"
  key_id    = "cert"
}

module "yav-secret-config_mqtt_cert" {
  source    = "../../modules/yav"
  secret_id = "sec-01d5xt5m7pnygvesvrz0nbvgv7"
  key_id    = "cert"
}

module "yav-secret-config_mqtt_key" {
  source    = "../../modules/yav"
  secret_id = "sec-01d5xt5m7pnygvesvrz0nbvgv7"
  key_id    = "key"
}

module "yav-secret-mqtt-tvm_token" {
  source    = "../../modules/yav"
  secret_id = "sec-01d6d95690enneb8qzve8114h6"
  key_id    = "tvmtool_token"
}

module "yav-secret-mqtt-tvm_secret" {
  source    = "../../modules/yav"
  secret_id = "sec-01d6d95690enneb8qzve8114h6"
  key_id    = "secret"
}

module "yav-secret-mqtt-tvm_client_id" {
  source    = "../../modules/yav"
  secret_id = "sec-01d6d95690enneb8qzve8114h6"
  key_id    = "client_id"
}


module "yav-secret-events-tvm_token" {
  source    = "../../modules/yav"
  secret_id = "sec-01dfvc4pggvngp0cffjq3hkx1h"
  key_id    = "tvmtool_token"
}

module "yav-secret-events-tvm_secret" {
  source    = "../../modules/yav"
  secret_id = "sec-01dfvc4pggvngp0cffjq3hkx1h"
  key_id    = "secret"
}

module "yav-secret-events-tvm_client_id" {
  source    = "../../modules/yav"
  secret_id = "sec-01dfvc4pggvngp0cffjq3hkx1h"
  key_id    = "client_id"
}

// selfping
module "yav-secret-config_mqtt_selfping_sub_cert" {
  source    = "../../modules/yav"
  secret_id = "sec-01dnhea6k6mx9gmceg7as65a7r"
  key_id    = "sub-cert"
}

module "yav-secret-config_mqtt_selfping_pub_cert" {
  source    = "../../modules/yav"
  secret_id = "sec-01dnhea6k6mx9gmceg7as65a7r"
  key_id    = "pub-cert"
}

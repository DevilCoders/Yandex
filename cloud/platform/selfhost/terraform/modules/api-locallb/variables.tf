variable "endpoint" {
}

variable "private_endpoint" {
}

variable "operation_service_id" {
}

variable "service_id" {
}

variable "id_prefix" {
}

variable "envoy_endpoints_string" {
}

variable "api_port" {
}

variable "hc_port" {
}

variable "private_api_port" {
}

variable "private_hc_port" {
}

variable "region" {
}

variable "zones" {
}

variable "private_api_cert_crt" {
  default = "/run/api/envoy/api.crt"
}

variable "private_api_cert_key" {
  default = "/run/api/envoy/api.key"
}

variable "trusted_ca" {
  default = "/usr/local/share/ca-certificates/YandexInternalRootCA.crt"
}

variable "metadata_version" {
}

variable "configserver_version" {
  default = "127c8a0162"
}

variable "configserver_pod_memory" {
}

variable "gateway_version" {
  default = "127c8a0162"
}

variable "gateway_pod_memory" {
}

variable "envoy_version" {
  default = "v1.14.4-12-g1844ad1"
}

variable "envoy_pod_memory" {
}

variable "envoy_heap_memory" {
}

variable "request_timeout" {
  default = "10s"
}

variable "num_retries" {
  default = 10
}

variable "per_try_timeout" {
}


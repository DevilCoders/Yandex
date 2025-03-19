variable "folder_id" {
  type = string
  description = "The ID of folder to deploy Mr. Prober"
  // Well-known id of "mr-prober" folder in "yc.vpc.monitoring" cloud. See https://bb.yandex-team.ru/projects/CLOUD/repos/bootstrap-templates/browse/terraform/*/vpc.tf
  default = "yc.vpc.mr-prober"
}

variable "control_network_hbf_enabled" {
  type = bool
  description = "Should HBF be enabled on Mr. Prober's control network. Should be false only for private clouds."
  default = true
}

variable "control_network_subnet_zones" {
  type = list(string)
  description = "List of zones for Mr. Prober's control network subnets."
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "control_network_ipv6_cidrs" {
  type = map(string)
  description = "IPv6 CIDRs of Mr. Prober's control network for each availability zones. Doesn't have default values, because there are differ set of prefixes on each zone. I.e. see http://netbox.cloud.yandex.net/ipam/prefixes/1269/ for TESTING VLA. Should include Project ID"
}

variable "control_network_ipv4_cidrs" {
  type = map(string)
  description = "IPv4 CIDRs of Mr. Prober's control network for each availability zones."
  default = {
    ru-central1-a = "172.16.0.0/16"
    ru-central1-b = "172.17.0.0/16"
    ru-central1-c = "172.18.0.0/16"
  }
}

variable "dns_zone" {
  type = string
  description = "DNS zone for all service VMs. I.e. prober.cloud.yandex.net"
}

variable "mr_prober_sa_name" {
  type = string
  description = "The name of mr-prober-sa service account. Should be unique in the cloud, so sometimes we need to specify a new name"
  default = "mr-prober-sa"
}

variable "mr_prober_sa_public_key" {
  type = string
  description = <<EOT
    Public part of RSA-4096 Authorized key for the service account. 
    Full key should be uploaded to https://yav.yandex-team.ru/secret/sec-01ekye608rtvgts7nbazvg1fv9/explore/versions.
    Can be generated via two commands:
      openssl genrsa -out mr_prober_sa.pem 4096
      openssl rsa -in mr_prober_sa.pem -outform PEM -pubout -out mr_prober_sa.pub.pem
EOT
}

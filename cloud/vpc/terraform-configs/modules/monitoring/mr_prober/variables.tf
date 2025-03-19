variable "ycp_profile" {
  type = string
  description = "Name of the ycp profile"
}

variable "yc_endpoint" {
  type = string
  description = "Endpoint for skm"
}

variable "folder_id" {
  type = string
  description = "The ID of folder to deploy Mr. Prober"
  // Well-known id of "mr-prober" folder in "yc.vpc.monitoring" cloud. See https://bb.yandex-team.ru/projects/CLOUD/repos/bootstrap-templates/browse/terraform/*/vpc.tf
  default = "yc.vpc.mr-prober"
}

variable "environment" {
  type = string
  description = "Name of cloud environment where Mr. Prober Control Plane is deployed, i.e. prod"
}

variable "mr_prober_environment" {
  type = string
  description = "Name of cloud environment which Mr. Prober Control Plane is responsible for, i.e. testing. Should be equal to var.environment in most cases"
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

variable "api_zones" {
  type = list(string)
  description = "List of zones for Mr. Prober's API."
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "api_domain" {
  type = string
  description = "Domain for Mr. Prober API, i.e. api.prober.cloud.yandex.net. Certificate will be issued for this domain, as well as ALB will be installed"
}

variable "api_vm_memory" {
  type = number
  description = "Number of GB RAM for API instances"
  default = 2
}

variable "api_vm_cores" {
  type = number
  description = "Number of cores for API instances"
  default = 2
}

variable "api_vm_fraction" {
  type = number
  description = "CPU fraction for API instances"
  default = 20
}

variable "api_alb_region" {
  type = string
  description = "Region for Application Load Balancer for API. I.e., ru-central1"
  default = "ru-central1"
}

variable "creator_vm_memory" {
  type = number
  description = "Number of GB RAM for Creator instance"
  default = 2
}

variable "creator_vm_cores" {
  type = number
  description = "Number of cores for Creator instance"
  default = 2
}

variable "creator_vm_fraction" {
  type = number
  description = "CPU fraction for Creator instance"
  default = 50
}

variable "creator_disk_size" {
  type = number
  description = "Disk size (in GB) for Creator instance"
  default = 50
}

variable "database_zones" {
  type = list(string)
  description = "List of zones for Mr. Prober's database hosts."
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "database_resource_preset" {
  type = string
  description = "Resource preset for Managed PostgreSQL"
  default = "s2.micro"
}

variable "run_creator" {
  type = bool
  description = "Should we run Creator instance or not?"
  default = false
}

variable "creator_zone_id" {
  type = string
  description = "Zone for Creator instance"
  default = "ru-central1-a"
}

variable "dns_zone_id" {
  type = string
  description = "The ID of DNS zone"
  default = "yc.vpc.mr-prober-dns-zone"
}

variable "dns_zone" {
  type = string
  description = "DNS zone for all service VMs. I.e. prober.cloud.yandex.net"
}

variable "dns_zone_enable_yandex_dns_sync" {
  type = bool
  description = "The flag enable_yandex_dns_sync of DNS zone. Should be true for almost all stands, false for Israel"
  default = true
}

variable "yav_database_secret_id" {
  type = string
  description = "ID of the yav secrets with database passwords"
  default = "sec-01exrkwcy0wpxagh9qy6snhpbx"
}

variable "hc_network_ipv6" {
  type = string
  description = "IPv6 healthcheck network"
}

variable "grpc_iam_api_endpoint" {
  type = string
  description = "Grpc endpoint of private IAM API. I.e., ts.private-api.cloud-preprod.yandex.net:4282. Can be obtained from `ycp [--profile PROFILE_NAME] config list | yq eval '.endpoints.iam.v1.services.iam-token.address' -`"
}

variable "grpc_compute_api_endpoint" {
  type = string
  description = "Grpc endpoint of private Compute API. I.e., compute-api.cloud-preprod.yandex.net:9051. Can be obtained from `ycp [--profile PROFILE_NAME] config list | yq eval '.endpoints.compute.v1.admin.services.node.address' -`"
}

variable "meeseeks_compute_node_prefixes_cli_param" {
  type = string
  description = "CLI parameter for yc-mr-prober-meeseeks tool. Following format is required: `--compute-node-prefix vla04-s7- --compute-node-prefix vla04-s9-`"
  default = ""
}

variable "mr_prober_sa_name" {
  type = string
  description = "The name of mr-prober-sa service account. Should be unique in the cloud, so sometimes we need to specify a new name"
  default = "mr-prober-sa"
}

variable "s3_endpoint" {
  type = string
  description = "Url for Object Storage aka S3 used on the stand. I.e. https://storage.yandexcloud.net"
  default = "https://storage.yandexcloud.net"
}

variable "s3_stand" {
  type = string
  description = "Stand name of Object Storage aka S3 used on the stand. I.e. prod (used for prod/preprod/testing) or israel (used for israel only)"
  default = "prod"
}

variable "use_conductor" { 
  type = bool
  description = "Create or not conductor entities for Control Plane"
  default = true
}

variable "platform_id" {
  type = string
  description = "Compute platform ID for instances"
  default = "standard-v2"
}

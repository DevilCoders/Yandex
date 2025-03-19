variable "folder_id" {
  type        = string
  description = "The ID of folder to deploy cluster"
  // Well-known id of "mr-prober" folder in "yc.vpc.monitoring" cloud. See https://bb.yandex-team.ru/projects/CLOUD/repos/bootstrap-templates/browse/terraform/*/vpc.tf
  default     = "yc.vpc.mr-prober"
}

variable "ycp_profile" {
  type        = string
  description = "Name of the ycp profile"
}

variable "yc_endpoint" {
  type        = string
  description = "Endpoint for public API (for yandex provider)"
}

variable "mr_prober_environment_override" {
  type        = string
  description = "Name of Mr. Prober environment if it differs from var.ycp_profile. I.e. 'hwlabs'"
  default     = ""
}

locals {
  mr_prober_environment = var.mr_prober_environment_override != "" ? var.mr_prober_environment_override : var.ycp_profile
}

variable "control_network_id" {
  type        = string
  description = "The id of Mr. Prober control network"
}

variable "control_network_subnet_ids" {
  type        = map(string)
  description = "Ids of Mr. Prober control network subnets for each availability zone"
}

variable "mr_prober_sa_id" {
  type        = string
  description = "The ID of Mr. Prober service account mr-prober-sa"
}

variable "mr_prober_secret_kek_id" {
  type        = string
  description = "The ID of Mr. Prober key for encrypting SKM metadata"
}

variable "dns_zone" {
  type        = string
  description = "DNS zone for all VMs in the cluster. I.e. prober.cloud.yandex.net"
}

variable "dns_zone_id" {
  type        = string
  description = "The ID of Mr. Prober DNS zone (defined in `dns_zone`)"
}

variable "cluster_id" {
  type        = number
  description = "The ID of Mr. Prober cluster"
}

variable "agent_additional_metric_labels" {
  type        = string
  description = "The dictionary with additional labels for solomon metrics saved in json."
  default     = "{}"
}

variable "mr_prober_conductor_group_name" {
  type        = string
  description = "[DEPRECATED: use EDS] The name of parent conductor group for all Mr. Prober clusters. I.e. cloud_preprod_mr_prober_clusters"
}

variable "iam_private_endpoint" {
  type        = string
  description = "Private endpoint of IAM data-plane for SKM. See https://wiki.yandex-team.ru/cloud/devel/platform-team/infra/skm/#konfiguracionnyjjfajjl. I.e., iam.private-api.cloud-preprod.yandex.net:4283"
}

variable "kms_private_endpoint" {
  type        = string
  description = "Private endpoint of KMS data-plane for SKM. See https://wiki.yandex-team.ru/cloud/devel/platform-team/infra/skm/#konfiguracionnyjjfajjl. I.e., kms.cloud-preprod.yandex.net:8443"
}

variable "mr_prober_container_registry" {
  type        = string
  description = "URL for Mr. Prober Container Registry"
  default     = "cr.yandex/crpni6s1s1aujltb5vv7"
}

variable "s3_endpoint" {
  type        = string
  description = "Url for Object Storage aka S3 used on the stand. I.e. https://storage.yandexcloud.net"
  default     = "https://storage.yandexcloud.net"
}

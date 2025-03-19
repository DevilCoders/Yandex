variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  // yc-cloud-trail/default
  default = "aoef3bqr00fuvnfaufbv"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1"
}

variable "service_account_id" {
  // yc-cloud-trail/sa-trail-tool
  default = "bfbi3tqslaot1vbu68rr"
}

variable "image_id" {
  default = "fdvilcqag97k44s9hovs"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "1"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"

  // VLA, MYT
  default = ["ru-central1-a"]
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:501:0:f81f:0:*
  // PREPROD IPs start with 120
  default = ["2a02:6b8:c0e:501:0:f81f:0:122"]
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.*
  // PREPROD IPs start with 120
  default = ["172.16.0.122"]
}

variable "subnets" {
    type = "map"

    default = {
        "ru-central1-a" = "buc2ob908hpbecsc16ue"
        "ru-central1-b" = "bltvvmg3lg3ahhc0urat"
        "ru-central1-c" = "fo2bnfaqlm244rpgs1og"
    }
}

variable "yc_zone_suffix" {
    default = {
        "ru-central1-a" = "vla"
        "ru-central1-b" = "sas"
        "ru-central1-c" = "myt"
    }
}

variable "hostname_suffix" {
  default = "trail.cloud-preprod.yandex.net"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "2"
}

variable "instance_core_fraction" {
    description = "Core fraction per one instance"
    default     = 20
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "4"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "20"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-hdd"
}

variable "host_group" {
  description = "SVM host group"
  default     = "service"
}

variable "instance_platform_id" {
  description = "instance platform id"
  default     = "standard-v2"
}

variable "security_group_ids" {
  default = ["c648jd8toaonmo63n057"]
}

// Parameters to override for alternative testing deploy in test.tfvars :

variable "name_prefix" {
    default = "preprod-tool"
    description = "display prefix name of instance"
}

variable "hostname_prefix" {
    default = "tool"
    description = "hostname prefix of instance"
}

variable "ydb_tablespace" {
  description = "additional subdirectory in ydb tablespace"
  default = ""
}

variable "logbroker_destination_installation" {
  default = "YC_LOGBROKER_PREPROD"
}

variable "logbroker_destination_topic_prefix" {
  default = "/aoedsmvctptsmbkj4g90/"
}

variable "logbroker_cloud_logging_topic" {
  default = "/yc.logs.cloud/yc-logging"
}

variable "logbroker_source_installation" {
  default = "YC_LOGBROKER_PREPROD"
}

variable "logbroker_source_topic" {
  default = "/aoedsmvctptsmbkj4g90/yc-events"
}

variable "logbroker_source_consumer" {
  default = "/aoedsmvctptsmbkj4g90/yc-trail-preparer"
}

variable "application_container_max_memory" {
  default = "2200Mi"
}

variable "application_jvm_xmx" {
  default = "1500m"
}

variable "app_log_level" {
    default = "debug"
}

variable "secondary_disk_name" {
  default = "log"
}

variable "secondary_disk_size" {
  default = "25"
}

variable "secondary_disk_type" {
  default = "network-hdd"
}

variable "ingest_api_host" {
  default = "ingest.trail.cloud-preprod.yandex.net"
}

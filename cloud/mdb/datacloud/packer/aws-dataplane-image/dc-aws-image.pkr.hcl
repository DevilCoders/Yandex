packer {
  required_version = "= 1.7.8"
  required_plugins {
    amazon = {
      version = ">= 0.0.1"
      source  = "github.com/hashicorp/amazon"
    }
  }
}

variable "image_type" {
  type    = string
  default = "ch"

  validation {
    condition     = contains(["ch", "zk", "kafka"], var.image_type)
    error_message = "Invalid image_type value."
  }
}

variable "environment" {
  type    = string
  default = "preprod"

  validation {
    condition     = contains(["prod", "preprod"], var.environment)
    error_message = "Invalid environment value."
  }
}

variable "subnet" {
  type    = string
  default = "subnet-0b57f72052452038d" # mdb-junk: default in eu-central-1a
}

variable "region" {
  type    = string
  default = "eu-central-1"
}

variable "source_ami_owners" {
  type    = list(string)
  default = ["099720109477"] # ubuntu ami owner
}

variable "architecture" {
  type    = string
  default = "amd64"

  validation {
    condition     = contains(["amd64", "arm64"], var.architecture)
    error_message = "Invalid architecture value."
  }
}

locals {
  timestamp   = regex_replace(timestamp(), "[- TZ:]", "")
  accessUsers = ["221736954043"] # dataplane prod account that should be able to copy images
  typeMapping = {
    ch = {
      name : "clickhouse"
      size : 20
    }
    zk = {
      name : "zookeeper"
      size : 10
    }
    kafka = {
      name : "kafka"
      size : 20
    }
  }
  envMapping = {
    preprod = {
      deploy_host = "deploy-api.preprod.mdb.internal.yadc.io"
      image_host  = "vm-image-template.yadc.io"
    }
    prod = {
      deploy_host = "deploy-api.prod.mdb.internal.double.tech"
      image_host  = "vm-image-template.at.double.cloud"
    }
  }
  instanceTypeMapping = {
    amd64 : "m5.large"
    arm64 : "c6g.large"
  }
  amiFilterMapping = {
    amd64 : "ubuntu/images/hvm-ssd/ubuntu-bionic-18.04-amd64-server-*"
    arm64 : "ubuntu/images/hvm-ssd/ubuntu-bionic-18.04-arm*"
  }

  image_params  = lookup(local.typeMapping, var.image_type, {})
  env_params    = lookup(local.envMapping, var.environment, {})
  instance_type = lookup(local.instanceTypeMapping, var.architecture, {})
  ami_filter    = lookup(local.amiFilterMapping, var.architecture, {})
}

source "amazon-ebs" "base-image" {
  ami_name      = "dc-aws-${var.architecture}-${local.image_params.name}-${local.timestamp}"
  instance_type = local.instance_type
  region        = var.region
  ami_regions = [
    "eu-central-1",
    "eu-west-1",
    "eu-west-2",
    "eu-west-3",
    "us-east-1",
    "us-east-2",
    "us-west-2",
    "ca-central-1",
    "sa-east-1",
    "ap-northeast-1",
    "ap-northeast-2",
    "ap-southeast-1",
    "ap-southeast-2",
    "ap-south-1"
  ]
  ami_users            = local.accessUsers
  snapshot_users       = local.accessUsers
  iam_instance_profile = "mdb-dataplane-node"
  subnet_id            = var.subnet
  ssh_interface        = "public_dns"
  run_tags = {
    map-migrated = "d-server-01fsq5a96hp704"
  }
  source_ami_filter {
    filters = {
      name                = local.ami_filter
      root-device-type    = "ebs"
      virtualization-type = "hvm"
    }
    most_recent = true
    owners      = var.source_ami_owners
  }
  ssh_username = "ubuntu"

  launch_block_device_mappings {
    device_name           = "/dev/sda1"
    volume_size           = local.image_params.size
    volume_type           = "gp2"
    delete_on_termination = true
  }
}

build {
  sources = ["source.amazon-ebs.base-image"]

  provisioner "file" {
    destination = "/tmp/"
    source      = "./conf/mdb-bionic.gpg"
  }

  provisioner "file" {
    destination = "/tmp/"
    source      = "./conf/master-sign.pub"
  }

  // for deploy api
  provisioner "file" {
    destination = "/tmp/"
    source      = "./conf/allCAs.pem"
  }

  provisioner "shell" {
    script          = "./scripts/prepare-base.sh"
    execute_command = "chmod +x {{ .Path }}; {{ .Vars }} sudo {{ .Path }} ${local.env_params.deploy_host} ${local.env_params.image_host}"
  }

  provisioner "shell" {
    script          = "./scripts/build-db.sh"
    execute_command = "chmod +x {{ .Path }}; {{ .Vars }} sudo {{ .Path }} ${var.image_type}"
  }

}


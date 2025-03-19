locals {
  timestamp = regex_replace(timestamp(), "[- TZ:]", "")
}

variable token {
  type = string
}

variable folder_id {
  type = string
}

variable subnet_id {
  type = string
}

variable zone {
  type = string
}

source "yandex" "rdpx" {
  token               = var.token
  folder_id           = var.folder_id
  source_image_family = "ubuntu-2004-lts"
  subnet_id           = var.subnet_id
  zone                = var.zone
  use_ipv4_nat        = true
  state_timeout       = "60m"

  instance_cores  = 2
  instance_mem_gb = 4

  image_name        = "ubuntu-20-04-lts-rdpx-v${local.timestamp}"
  image_family      = "ubuntu-20-04-lts-rdpx"
  image_description = "Ubuntu 20.04 LTS with rdpx binaries"

  communicator = "ssh"
  ssh_username = "ubuntu"

  ssh_clear_authorized_keys = true
}

build {
  name        = "rdpx"
  description = "This build creates updated ubuntu 20.04 LTS with prepared for use rdpx"
  sources     = ["yandex.rdpx"]

  provisioner shell {
    execute_command = "sudo {{ .Vars }} bash '{{ .Path }}'"
    scripts = [
      "../scripts/update.sh",
      "../scripts/ubuntu-desktop.sh",
      "../scripts/xrdp.sh",
      "../scripts/cleanup.sh"
    ]
  }

  post-processors {
    post-processor manifest {
      output     = "${build.name}-manifest.json"
      strip_path = true
      custom_data = {
        source_image_id = "${build.SourceImageID}"
      }
    }
  }
}

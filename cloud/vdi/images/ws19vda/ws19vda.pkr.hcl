locals {
  password              = "${ split("-", uuidv4())[4] }P@sS!1" # random pwd + special charecters
  timestamp             = regex_replace(timestamp(), "[- TZ:]", "")
  desktop_agent_folder  = "C:\\Program Files\\Yandex.Cloud\\Cloud Desktop"
  setup_complete_folder = "C:\\Windows\\Setup\\Scripts"
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

source yandex ws19vda {
  token               = var.token
  folder_id           = var.folder_id
  source_image_family = "windows-2019-dc-gvlk"
  subnet_id           = var.subnet_id
  zone                = var.zone
  use_ipv4_nat        = true
  state_timeout       = "60m"

  instance_cores  = 4
  instance_mem_gb = 8
  disk_size_gb    = 50

  image_name        = "windows-server-2019-datacenter-clouddesktop-v${local.timestamp}"
  image_family      = "windows-server-2019-datacenter-clouddesktop"
  image_description = "Windows Server 2019 Datacenter with Cloud Desktop Agent"

  communicator   = "winrm"
  winrm_username = "Administrator"
  winrm_password = "${ local.password }"
  winrm_use_ssl  = true
  winrm_insecure = true
  winrm_use_ntlm = true

  metadata = {
    user-data = "#ps1\nnet user Administrator ${ local.password }"
  }
}

build {
  name = "ws19cd"
  sources = [
    "source.yandex.ws19vda"
  ]

  # Desktop Agent

  provisioner powershell {
    inline = ["mkdir \"${local.desktop_agent_folder}\" -f"]
  }

  provisioner file {
    source      = "../../DesktopAgent.exe"
    destination = "${local.desktop_agent_folder}\\DesktopAgent.exe"
  }

  provisioner powershell {
    script = "../scripts/Install-DesktopAgent.ps1"
  }

  # Sysprep

  provisioner powershell {
    inline = ["mkdir \"${local.setup_complete_folder}\" -f"]
  }

  provisioner file {
    source      = "../scripts/unattend/"
    destination = "${local.setup_complete_folder}"
  }

  provisioner powershell {
    script = "../scripts/Sysprep.ps1"
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

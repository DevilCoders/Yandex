
# Some configs wich can be overriden
variables {
  source_family          = "paas-base-g4"
  source_image           = "paas-base-g4-20220202122207"
  source_image_folder_id = "b1grpl1006mpj1jtifi1" #paas-images@yc-ycloud-deploy
  cr_repo                = "cr.yandex/crpb8mold3ptghdke34l"
}

variable "zone" {
  type = string
}

variable "folder_id" {
  type = string
}

variable "subnet_id" {
  type = string
}

variable "build_sa" {
  type = string
}

variable "config_image_version" {
  type = string
}

variable "tools_image_version" {
  type = string
}

variable "piper_image_version" {
  type = string
}

variable "output_image_name" {
  type = string
}

locals {
  config_image = "${var.cr_repo}/yc-billing-piper-configs:${var.config_image_version}"
  tools_image  = "${var.cr_repo}/yc-billing-sidecars:${var.tools_image_version}"
  piper_image  = "${var.cr_repo}/yc-billing-piper:${var.piper_image_version}"
  mem_limit    = "10Gi"
}

# Build configuration
source "yandex" "build-piper" {
  zone                   = "${var.zone}"
  folder_id              = "${var.folder_id}"
  subnet_id              = "${var.subnet_id}"
  service_account_id     = "${var.build_sa}"
  source_image_name      = "${var.source_image}"
  source_image_folder_id = "${var.source_image_folder_id}"
  ssh_username           = "ubuntu"
  use_ipv4_nat           = false
  use_ipv6               = true

  image_family      = "biling-piper"
  image_name        = "${var.output_image_name}"
  image_description = "billing piper image"

  disk_type = "network-hdd"
  labels = {
    "skip_update_ssh_keys" = "true"
  }
  # Add metadata if you want to debug on alive vm. Use packer build -on-error=abort
  # NOTE: password is "q"
  #   metadata = {
  #     user-data = <<EOT
  # #cloud-config
  # users:
  #   - name: debug
  #     sudo: ALL=(ALL) NOPASSWD:ALL
  #     shell: /bin/bash
  #     passwd: $6$rounds=4096$ej5Z5As2j5Pb91i$mw93Tbx8I6zOIW1S5DY.1N0yRjvjuJejQUomQqixe/ZpzljBQY6bf3qn5N2XssKP2jBHOv7H7yD9QRN0pmZPR/
  #     lock_passwd: false
  # EOT
  #   }
}

locals {
  pod-content = templatefile("${path.root}/files/piper.pod.tmpl", {
    config_image : local.config_image,
    tools_image : local.tools_image,
    piper_image : local.piper_image,
    mem_limit: local.mem_limit,
  })
}


# Build steps
build {
  name    = "billing-piper"
  sources = ["source.yandex.build-piper"]

  provisioner "shell-local" {
    inline = [
      "echo ${var.output_image_name} > version.txt",
    ]
  }

  # network not so fast for grant access to yandex nets - be patient and wait
  provisioner "shell" {
    inline = [
      "sleep 10",
    ]
  }

  provisioner "file" {
    source = "${path.root}/files/fluentbit-yc-output.conf"
    destination = "/tmp/yc-output.conf"
  }

  provisioner "file" {
    source = "${path.root}/files/piper-control"
    destination = "/tmp/piper-control"
  }

  provisioner "file" {
    source = "${path.root}/files/juggler"
    destination = "/tmp/piper-juggler"
  }

  # prepare base env
  # some settings and prints
  provisioner "shell" {
    inline = [
      "sudo systemctl disable salt-minion",
      "sudo systemctl stop kubelet",
      "echo -n 'image prepared by Packer at ' | sudo tee /etc/image_description",
      "date --rfc-3339 sec | sudo tee -a /etc/image_description",

      "echo 'Content of /etc/resolv.conf:' ; cat /etc/resolv.conf",
      "echo 'Content of /etc/hosts:' ; cat /etc/hosts",
    ]
  }

  # images
  provisioner "shell" {
    execute_command = "chmod a+x {{ .Path }}; {{ .Vars }} sudo {{ .Path }}"
    inline = [
      "docker ps -aq | xargs docker rm",
      "docker images -q | grep -v $(docker images k8s.gcr.io/pause -q) | xargs docker rmi",
      "docker pull ${local.config_image}",
      "docker pull ${local.tools_image}",
      "docker pull ${local.piper_image}",
    ]
  }

  # pods
  provisioner "shell" {
    execute_command = "chmod a+x {{ .Path }}; {{ .Vars }} sudo {{ .Path }}"
    inline = [
      "cd /etc/kubelet.d/",
      "ls -1|xargs rm", # Remove all implicit kubelets
      "cat > /etc/kubelet.d/piper.pod <<EOFSTR\n${local.pod-content}\nEOFSTR",
      "echo 'secrets:' > /etc/skm.yaml", # skm service default dependecy
    ]
  }

  # logs, scripts, bundle
  provisioner "shell" {
    execute_command = "chmod a+x {{ .Path }}; {{ .Vars }} sudo {{ .Path }}"
    inline = [
      "mv /tmp/yc-output.conf /etc/td-agent-bit/conf.d/yc-output.conf",
      "ln -s /etc/td-agent-bit/conf.d/yc-output.conf /etc/td-agent-bit/enabled/yc-output.conf", # skm service default dependecy

      "chmod a+x /tmp/piper-control",
      "mv /tmp/piper-control /usr/sbin/",

      # relocate standard bundle and add our one
      "mkdir -p  /juggler-bundle/paas",
      "mv /juggler-bundle/*.* /juggler-bundle/paas/",
      "chmod a+x /tmp/piper-juggler/*.sh",
      "mv /tmp/piper-juggler /juggler-bundle/piper",
    ]
  }

  # cleanup
  provisioner "shell" {
    execute_command = "chmod a+x {{ .Path }}; {{ .Vars }} sudo {{ .Path }}"
    inline = [
      "rm -f /etc/ssh/*_key*",
      "rm -f /etc/machine-id",
      "touch /etc/machine-id",
      "rm /var/lib/dhcp/* || true",
      "rm -rf /tmp/*",
      "unset HISTFILE",
      "rm -f /root/.bash_history",
      "rm -rf /root/.ssh/*",
      "rm -f /root/*",
      "find /var/log -type f -delete",
    ]
  }

  # TODO: juggler + bundle

  post-processor "manifest" {
    output     = "yandex-export-manifest.json"
    strip_path = true
  }
}

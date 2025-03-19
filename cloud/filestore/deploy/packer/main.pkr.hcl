source "yandex" "server" {
  endpoint = var.endpoint
  token    = var.token

  zone               = var.zone_id
  folder_id          = var.folder_id
  subnet_id          = var.subnet_id
  service_account_id = var.service_account_id

  source_image_id = var.source_image_id
  disk_type       = "network-nvme"

  image_family      = "yandex-cloud-filestore-server"
  image_name        = "yandex-cloud-filestore-server-${var.image_version}"
  image_description = "Build by ${var.user}"

  image_labels = {
    #app_version     = var.app_version
    source_image_id = var.source_image_id
  }

  labels = {
    abc_svc              = "ycnbs"
    env                  = var.env
    skip_update_ssh_keys = true
  }

  metadata = {
    osquery_tag         = "ycloud-svc-filestore-server"
    serial_port_enabled = 1
  }

  preemptible  = true
  use_ipv4_nat = false
  use_ipv6     = true

  ssh_bastion_agent_auth = var.bastion_agent_auth
  ssh_bastion_host       = var.bastion_host
  ssh_bastion_username   = var.user
  ssh_username           = "ubuntu"

  state_timeout = "15m"
}

source "yandex" "nfs" {
  endpoint = var.endpoint
  token    = var.token

  zone               = var.zone_id
  folder_id          = var.folder_id
  subnet_id          = var.subnet_id
  service_account_id = var.service_account_id

  source_image_id = var.source_image_id
  disk_type       = "network-nvme"

  image_family      = "yandex-cloud-filestore-nfs"
  image_name        = "yandex-cloud-filestore-nfs-${var.image_version}"
  image_description = "Build by ${var.user}"

  image_labels = {
    #app_version     = var.app_version
    source_image_id = var.source_image_id
  }

  labels = {
    abc_svc              = "ycnbs"
    env                  = var.env
    skip_update_ssh_keys = true
  }

  metadata = {
    osquery_tag         = "ycloud-svc-filestore-nfs"
    serial_port_enabled = 1
  }

  preemptible  = true
  use_ipv4_nat = false
  use_ipv6     = true

  ssh_bastion_agent_auth = var.bastion_agent_auth
  ssh_bastion_host       = var.bastion_host
  ssh_bastion_username   = var.user
  ssh_username           = "ubuntu"

  state_timeout = "15m"
}

build {
  sources = ["yandex.server", "yandex.nfs"]

  provisioner "shell" {
    inline = [
      "echo Prepare Salt grains",
      "echo 'app_image: ${var.docker_registry}/yandex-cloud-filestore-${source.name}' | sudo tee -a /etc/salt/grains",
      "echo 'app_version: ${var.app_version}' | sudo tee -a /etc/salt/grains"
    ]
  }

  provisioner "salt-masterless" {
    local_state_tree = "${path.root}/salt"
    salt_call_args   = "--id ${source.name}"
    skip_bootstrap   = true
  }

  provisioner "shell" {
    inline = [
      "echo 'Purge Salt packages'",
      "sudo apt-get -y purge salt-common salt-minion",
      "sudo rm -rf /srv/salt",
      "sudo rm -rf /var/cache/salt"
    ]
  }

  provisioner "shell" {
    inline = [
      "echo 'Cleanup docker credentials'",
      "sudo rm -f /home/ubuntu/.docker/config.json"
    ]
  }

  post-processor "manifest" {
    output     = "${path.cwd}/yandex-builder-manifest.json"
    strip_path = true
  }

  post-processor "yandex-export" {
    endpoint = var.endpoint
    token    = var.token

    zone               = var.zone_id
    folder_id          = var.folder_id
    subnet_id          = var.subnet_id
    service_account_id = var.service_account_id

    disk_type = "network-hdd"

    preemptible  = true
    use_ipv4_nat = false
    use_ipv6     = true

    paths = [
      "s3://${var.bucket_id}/${build.ImageName}.qcow2"
    ]

    keep_input_artifact = true
  }

  post-processor "manifest" {
    output     = "${path.cwd}/yandex-export-manifest.json"
    strip_path = true
  }
}

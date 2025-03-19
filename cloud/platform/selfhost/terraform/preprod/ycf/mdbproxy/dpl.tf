locals {
  dpl_image           = "fdvqmaltb0sm66mr4o3c"

  dpl_service_account = "bfbfk5ijkbrbqhoqj0as"
}

module dpl {
    source = "../ig-ycp"

    name        = "mdbproxy-dpl"
    description = "MDBProxy-DPL IG for postgre"
    service     = "mdbproxy-dpl"

    amount = 1
    cores  = 2
    memory = 4
    core_fraction = 20
    platform_id="standard-v2"

    boot_disk_image_id = local.dpl_image

    folder_id                   = local.yc_folder
    ssh-keys                    = module.ssh-keys.ssh-keys
    conductor_group             = "serverless-mdbproxy-dpl"
    service_account_id          = local.yc_ig_sacc
    instance_service_account_id = local.dpl_service_account
    subnets                     = local.subnets
    instance_metadata = merge(local.arbitrary_metadata, {
        attrs      = <<-EOF
    internal_dc
  EOF
        ssh-keys   = module.ssh-keys.ssh-keys
        mdbproxy_production = "false"
        mdbproxy_metrics_subsystem = "mdbproxydpl"
        mdbproxy_shardid = "postgre1.serverless.cloud-preprod.yandex.net"
        launch_index = "{instance.index}"
    })
}


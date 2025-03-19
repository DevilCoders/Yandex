locals {
  configs = [
    for component in var.components :
    component.configs
  ]

  components_envs = [
    for component in var.components :
    component.envvar
  ]

  components_volumes = [
    for component in var.components :
    component.volumes
  ]

  all_containers = [
    for component in var.components :
    component.container
  ]

  required_volumes = toset(flatten([
    for container in local.all_containers :
    length(container.mounts) != 0 ? keys(container.mounts) : []
  ]))

  components_volumes_merged = merge(local.components_volumes...)

  /*
   * TODO: We merge all volumes in one list
   *       Desired behavior: use additional_volumes specified by component
   *       only in this component itself not others
   */
  provided_volumes = merge(local.components_volumes_merged, local.standard_volumes, var.additional_volumes)

  volumes = {
    for required_volume in local.required_volumes :
    required_volume => local.provided_volumes[required_volume]
  }

  required_env = toset(flatten([
    for container in local.all_containers :
    length(container.envvar) != 0 ? container.envvar : []
  ]))

  /*
   * TODO: We merge all environments in one list
   *       Desired behavior: use additional_envvar specified by component
   *       only in this component itself not others
   */
  components_env = merge(local.components_envs...)

  provided_env = merge(local.components_env, local.standard_environment, var.additional_envvar)

  env_vars = {
    for required_var in local.required_env :
    required_var => local.provided_env[required_var]
  }

  instance_group_name = "${var.name}-${var.environment}"

  eth1_routes = length(var.networks) > 1 ? [
    for cidr in var.networks[1].interface.routes : "pre-up ip -6 route replace ${cidr} via fe80::1 dev $IFACE"
  ] : []
}

module "constants" {
  source      = "../../constants"
  environment = var.environment
}

module "standard_service_configs" {
  source = "../configs_generator"

  input_configs = local.configs
}

module "configs" {
  source = "../configs_generator"

  input_configs = [
    for component in var.components :
    component.configs
  ]
}

module "user_data" {
  source       = "../user_data"
  yandex_token = var.yandex_token
  extra_bootcmd = [
    ["bash", "-c", "/usr/bin/skm decrypt"],
  ]
  extra_runcmd = concat([
    ["bash", "-c", "/usr/bin/skm decrypt"],

    # FIXME: This command is valid only for hosts with two interfaces
    #        Also it is better to use https://cloudinit.readthedocs.io/en/latest/topics/network-config.html#network-configuration
    ["bash", "-c", "printf 'auto eth1\niface eth1 inet6 manual\n    ${join("\n    ", local.eth1_routes)}\n    pre-up /sbin/ip link set $IFACE mtu 8950\n    up /usr/bin/yc-inet6-cfg $IFACE up -d 0\n    down /usr/bin/yc-inet6-cfg $IFACE down\n' > /etc/network/interfaces.d/20_yc_additional_cfg && ifup eth1"]
    ],
  var.additional_runcmd)

  extra_user_data = var.additional_userdata
  
  abc_service = var.abc_service
}

module "docker_config" {
  source       = "../docker_config"
  yandex_token = var.yandex_token
  secret       = module.constants.by_environment.common_secret
}

module "standard_service_podmanifest" {
  source = "../podmanifest_generator"

  name          = var.name
  config_digest = sha256(module.configs.configs_rendered)
  containers    = local.all_containers
  volumes       = local.volumes
  envvar        = local.env_vars
}

module "instance_group" {
  source = "../standard_instance_group"

  // Secrets passthough
  folder_id          = var.folder_id
  service_account_id = var.service_account_id

  // Target
  environment = var.environment

  // Instance group description
  name = local.instance_group_name
  # description         = data.template_file.ig_description.rendered
  # TODO: render using header
  description = local.instance_group_name

  // Resources description
  networks               = var.networks
  zones                  = var.zones
  instance_group_size    = var.instance_group_size
  resources_per_instance = var.resources_per_instance
  boot_disk_spec = {
    size = var.instance_disk_desc.size
    type = var.instance_disk_desc.type
    image_id = coalesce(
      var.image_id,
      var.resources_per_instance.gpus == 0
      ? module.constants.by_environment.kubelet_cpu_image
      : module.constants.by_environment.kubelet_gpu_image
    )
  }

  secondary_disk_specs = var.secondary_disk_specs

  deploy_policy = var.deploy_policy

  // Service config
  docker_config  = module.docker_config.rendered
  podmanifest    = module.standard_service_podmanifest.podmanifest_rendered
  configs        = module.configs.configs_rendered
  secrets_bundle = var.skm_bundle
  user_data      = module.user_data.rendered

  nlb_target = var.nlb_target

  // External  
  target_group_name        = var.target_group_name
  target_group_description = var.target_group_description
}

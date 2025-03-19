locals {
  api_version = "v1"

  init_containers = [
    for container in var.containers :
    container
    if container.is_init
  ]

  containers = [
    for container in var.containers :
    container
    if !container.is_init
  ]

  _pod_init_containers_mounts_validation = [
    for init_container in local.init_containers : [
      for mount_name, mount_desc in init_container.mounts :
      var.volumes[mount_name]
    ]
  ]

  pod_init_containers_1 = [
    for init_container in local.init_containers :
    {
      orig = init_container,
      res = {
        name  = init_container.name
        image = init_container.image
        ports = [
          for port in init_container.ports :
          { containerPort = port }
        ]

        volumeMounts = [
          for mount_name, mount_desc in init_container.mounts :
          {
            name      = mount_name
            mountPath = mount_desc.path
            readOnly  = tobool(lookup(mount_desc, "read_only", true))
          }
        ]
      }
    }
  ]

  pod_init_containers_with_command = [
    for container in local.pod_init_containers_1 :
    merge(container.res, {
      command = container.orig.command
      args    = container.orig.args
    })
    if length(container.orig.command) != 0
  ]

  pod_init_containers_without_command = [
    for container in local.pod_init_containers_1:
      container.res
    if length(container.orig.command) == 0
  ]

  pod_init_containers = concat(
    local.pod_init_containers_with_command,
    local.pod_init_containers_without_command
  )

  _pod_containers_mounts_validation = [
    for container in local.containers: [
      for mount_name, mount_desc in container.mounts:
      var.volumes[mount_name]
    ]
  ]

  pod_containers_1 = [
    for container in local.containers:
    {
      orig = container
      res = {
        name  = container.name
        image = container.image
        ports = [
          for port in container.ports:
          { containerPort = port }
        ]

        volumeMounts = [
          for mount_name, mount_desc in container.mounts:
          {
            name      = mount_name
            mountPath = lookup(mount_desc, "path", "some_path")
            readOnly  = tobool(lookup(mount_desc, "read_only", true))
          }
        ]

        env = [
          for envvar in container.envvar:
          {
            name  = envvar
            value = var.envvar[envvar]
          }
        ]
      }
    }
  ]

  pod_containers_with_command = [
    for container in local.pod_containers_1:
    merge(container.res, {
      command = container.orig.command
      args    = container.orig.args
    })
    if length(container.orig.command) != 0
  ]

  pod_containers_without_command = [
    for container in local.pod_containers_1:
    container.res
    if length(container.orig.command) == 0
  ]

  pod_containers = concat(
    local.pod_containers_with_command,
    local.pod_containers_without_command
  )

  pod_volumes = [
    for volume_name, volume_desc in var.volumes :
    {
      name     = volume_name
      hostPath = volume_desc.hostPath
    }
  ]

  pod_metadata = {
    name      = var.name
    namespace = "kube-system"
    annotations = {
      config_digest = var.config_digest
      # something about this
      # scheduler.alpha.kubernetes.io/critical-pod: ""
    }
  }

  pod_spec = {
    priority                      = 2000000001
    priorityClassName             = "system-cluster-critical"
    hostNetwork                   = true
    terminationGracePeriodSeconds = 300
    initContainers                = local.pod_init_containers
    containers                    = local.pod_containers
    volumes                       = local.pod_volumes
  }

  pod = {
    apiVersion = local.api_version
    kind       = "Pod"
    metadata   = local.pod_metadata
    spec       = local.pod_spec
  }

  podmanifest_items = [
    local.pod
  ]

  podmanifest = {
    apiVersion = local.api_version
    kind       = "PodList"
    items      = local.podmanifest_items
  }
}
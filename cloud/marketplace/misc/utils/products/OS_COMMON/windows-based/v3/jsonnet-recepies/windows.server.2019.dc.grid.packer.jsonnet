local builders = import '../jsonnet-lib/packer-windows-builders.libsonnet';
local variables = import '../jsonnet-lib/packer-windows-variables.libsonnet';
local provisioners = import '../jsonnet-lib/packer-windows-provisioners.libsonnet';
local post_processors = import '../jsonnet-lib/packer-common-postprocessors.libsonnet';

{
  'variables': variables.windows_vgpu {
    //source_image_name: 'windows-2019-dc-base-v20200824',
    source_image_family: 'windows-2019-dc-base',
    image_description: 'Windows Server 2019 Datacenter with Nvidia GRID drivers',
    image_family: 'windows-2019-gvlk-vgpu',
  },
  'builders': builders.windows_vgpu,
  'provisioners': provisioners.vgpu,
  'post-processors': post_processors.fabric_common,
}
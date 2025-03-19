local builders = import '../jsonnet-lib/packer-windows-builders.libsonnet';
local variables = import '../jsonnet-lib/packer-windows-variables.libsonnet';
local provisioners = import '../jsonnet-lib/packer-windows-provisioners.libsonnet';
local post_processors = import '../jsonnet-lib/packer-common-postprocessors.libsonnet';

{
  'variables': variables.windows_gpu {
    //source_image_name: 'windows-2016-dc-base-v20200824',
    source_image_family: 'windows-2016-dc-base',
    image_description: 'Microsoft Windows Server 2016 Datacenter with Nvidia CUDA 11',
    image_family: 'windows-2016-gvlk-gpu',
  },
  'builders': builders.windows_gpu,
  'provisioners': provisioners.gpu,
  'post-processors': post_processors.fabric_common,
}
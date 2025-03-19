local builders = import '../jsonnet-lib/packer-windows-builders.libsonnet';
local variables = import '../jsonnet-lib/packer-windows-variables.libsonnet';
local provisioners = import '../jsonnet-lib/packer-windows-provisioners.libsonnet';
local post_processors = import '../jsonnet-lib/packer-common-postprocessors.libsonnet';

{
  'variables': variables.windows_server {
    //source_image_name: 'windows-2016-std-base-v20200824',
    source_image_family: 'windows-2016-std-base',
    image_description: 'Microsoft Windows Server 2016 Standard',
    image_family: 'windows-2016-std-gvlk',
  },
  'builders': builders.windows_common,
  'provisioners': provisioners.ws,
  'post-processors': post_processors.fabric_common,
}
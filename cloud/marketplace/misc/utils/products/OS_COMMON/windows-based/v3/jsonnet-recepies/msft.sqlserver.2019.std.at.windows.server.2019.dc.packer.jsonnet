local builders = import '../jsonnet-lib/packer-windows-builders.libsonnet';
local variables = import '../jsonnet-lib/packer-windows-variables.libsonnet';
local provisioners = import '../jsonnet-lib/packer-windows-provisioners.libsonnet';
local post_processors = import '../jsonnet-lib/packer-common-postprocessors.libsonnet';

{
  'variables': variables.windows_sqlserver {
    //source_image_name: 'windows-2016-dc-base-v20200824',
    source_image_family: 'windows-2019-dc-base',
    image_family: 'sqlserver-2019-std-win2019',
    image_description: 'Microsoft SQL Server 2019 Standard',
    sql_version: 'sql-2019-standard',
  },
  'builders': builders.windows_common,
  'provisioners': provisioners.sqlserver,
  'post-processors': post_processors.fabric_common,
}
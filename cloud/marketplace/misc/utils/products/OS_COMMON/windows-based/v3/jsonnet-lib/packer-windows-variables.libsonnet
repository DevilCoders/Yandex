local windows_common = {
    endpoint: '{{env `YC_ENDPOINT`}}',
    //service_account_key_file: '{{env `YC_BUILD_SERVICE_ACCOUNT_SECRET`}}',
    mkt_s3_access_key: '{{env `YC_MKT_DISTR_S3_ACCESS_KEY`}}',
    mkt_s3_secret_key: '{{env `YC_MKT_DISTR_S3_SECRET_KEY`}}',

    source_image_family: error '"windows_common" must have "source_image_family" field',
    source_image_folder_id: '{{env `YC_DIRTY_IMAGES_FOLDER_ID`}}',
    
    image_family: error '"windows_common" must have "image_family" field',
    image_description: error '"windows_common" must have "image_description" field',
    platform: error '"windows_common" must have "platform" field',

    folder_id: '{{env `YC_BUILD_FOLDER_ID`}}',
    zone: '{{env `YC_ZONE`}}',
    subnet_id: '{{env `YC_BUILD_SUBNET`}}',

    username: 'Administrator',
    generated_password: '{{ split uuid \"-\" 4 }}P@sS!1',
};

local windows_rdls = windows_common {
  source_image_family: 'windows-2019-dc-base',
  //source_image_name: 'windows-2019-dc-base-v20200824',
  licenses_count: error '"windows_rdls" must have "source_image_name" field',
  agreement_number: '88637253',
  image_family: 'windows-2019-dc-gvlk-rds-' + std.toString(self.licenses_count),
  image_description: 'Microsoft Windows Server 2019 Datacenter with ' + std.toString(self.licenses_count) + ' RDS licenses',
  platform: 'standard-v2',
};

local windows_sqlserver = windows_common {
  //source_image_family: error '"windows_sqlserver" must have "source_image_family" field',
  sql_version: error '"windows_sqlserver" must have "sql_version" field',
  platform: 'standard-v2',
};

local windows_gpu = windows_common {
  //source_image_family: error '"windows_gpu" must have "source_image_family" field',
  //source_image_name: error '"windows_gpu" must have "source_image_name" field',
  platform: 'gpu-standard-v1',
};

local windows_vgpu = windows_common {
  //source_image_family: error '"windows_vgpu" must have "source_image_family" field',
  //source_image_name: error '"windows_vgpu" must have "source_image_name" field',
  platform: 'vgpu-standard-v1',
};

local windows_server = windows_common {
  //source_image_family: error '"windows_server" must have "source_image_family" field',
  platform: 'standard-v2',
};

local windows_base = windows_common {
  //source_image_name: error '"windows_base" must have "source_image_name" field',
  target_image_folder_id: error '"windows_base" must have "target_image_folder_id" field',
  platform: 'standard-v2',
};

{
  windows_common: windows_common,
  windows_rdls: windows_rdls,
  windows_sqlserver: windows_sqlserver,
  windows_gpu: windows_gpu,
  windows_vgpu: windows_vgpu,
  windows_server: windows_server,
  windows_base: windows_base,
}

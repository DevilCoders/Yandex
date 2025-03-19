# creating base images

## How it looks

1. bootstrap
2. ensure-smb1
3. ensure-nla
4. ensure-eth
5. ensure-icmp
6. ensure-rdp
7. ensure-rtu
8. ensure-serialconsole
9. ensure-powerplan
10. ensure-shutdownwithoutlogon
11. ensure-updated
12. windows-restart
13. ensure-ngen
14. ensure-cleanup
15. noop w8t (sometimes windows is forcibly stopped)

## Snippets

* for direct copying to 'dirty images'

Variables section: `"target_image_folder_id": "{{env `YC_DIRTY_IMAGES_FOLDER_ID`}}",`
Builder section: `"source_image_folder_id": "{{user `source_image_folder_id`}}",`

* for diagnostics runs:

Provisioner step `{ "type": "shell-local", "inline": ["echo \"pwd is {{ user `generated_password` }}\""] },` will print password.
Provisioner step `{ "type": "breakpoint", "note": "shazam!" }` will w8t for you, bone bag.

* cloning images (ad-hoc image moving one)

```(powershell)
yc config profile create mkt-dirty-images \
  --cloud-id b1gjqgj3hhvjen5iqakp \
  --folder-id b1g6fufm5hslknc6vvdk
yc config profile activate mkt-dirty-images

SOURCE_IMAGE_FOLDER_ID=b1ggn218t7imq4mig0lg # mkt-test
SOURCE_IMAGE_NAME=""

yc compute image create \
  --name $WS19DC_IMAGE_NAME
  --source-image-name $SOURCE_IMAGE_NAME \
  --source-image-folder-id $SOURCE_IMAGE_FOLDER_ID \
  --async
```

* cloning images (ad-hoc image moving all at once)

```(powershell)
function Clone-YCImages {
# usage
#
# $SOURCE_IMAGE_FOLDER_ID = "b1ggn218t7imq4mig0lg" # mkt-test
# $TARGET_IMAGE_FOLDER_ID = "b1g8fqssmhljjerknber" # mkt-dirty-images
# $TYPE = "base"
# $DATE = "200821-1450" # "v20200821"
#
# Clone-YCImages $SOURCE_IMAGE_FOLDER_ID $TARGET_IMAGE_FOLDER_ID $TYPE $DATE
  param(
    $source_folder_id,
    $target_folder_id,
    $type,
    $date,
    $prefixes = @(
      "windows-2019-dc",
      "windows-2019-std",
      "windows-2016-dc",
      "windows-2016-std",
      "windows-2012r2-dc",
      "windows-2012r2-std"
    )
  )

  $configured_yc_folder_id = (& yc config list | sls folder-id | select -exp Line).split(" ")[-1] # --format json not respected
  if ($target_folder_id -ne $configured_yc_folder_id) { throw "active yc profile is $configured_yc_folder_id, but $target_folder_id expected" }

  $source_folder_name = yc resource folder get $source_folder_id --format json | ConvertFrom-Json | Select-Object -ExpandProperty name
  if ( [string]::IsNullOrEmpty($source_folder_name) ) { throw "can't get $source_folder_id name" }

  $target_folder_name = yc resource folder get $target_folder_id --format json | ConvertFrom-Json | Select-Object -ExpandProperty name
  if ( [string]::IsNullOrEmpty($target_folder_name) ) { throw "can't get $target_folder_id name" }

  # quota's is not in public api

  foreach ($prefix in $prefixes) {
    if ($type) { $i = $prefix + "-" + $type + "-" + $date }
    else { $i = $prefix + "-" + $date }

    $s = & yc compute image get $i --folder-id $source_folder_id  --format json | ConvertFrom-Json | Select-Object -ExpandProperty name    
    if ( [string]::IsNullOrEmpty($s) ) { throw "can't get image $i from $source_folder_name ($source_folder_id) " }

    $t = & yc compute image get $i --format json | ConvertFrom-Json | Select-Object -ExpandProperty name | Out-Null
    if ( -not [string]::IsNullOrEmpty($t) ) { throw "folder $target_folder_name ($target_folder_id) already contains image $i" }

    Write-Host "Copying $i from $source_folder_name ($source_folder_id) to $target_folder_name ($target_folder_id)"

    & yc compute image create `
      --name $i `
      --source-image-name $i `
      --source-image-folder-id $source_folder_id `
      --async
  }
}
```

* test'em all

```(powershell)
function Create-YCBaseInstances {
  param(    
    [ValidateNotNullOrEmpty()]
    [string]$userdata_path = "",

    [ValidateRange(0,16)]
    [int]$memory_gb = 4,
    
    [ValidateRange(0,16)]
    [int]$cores = 2,
    
    [ValidateNotNullOrEmpty()]
    [string]$subnet_name = "cloudmkttestnets-ru-central1-a",

    $type = "base",
    $date = "200821-1405", # "v20200821"
    $prefixes = @(
      "windows-2019-dc",
      "windows-2019-std",
      "windows-2016-dc",
      "windows-2016-std",
      "windows-2012r2-dc",
      "windows-2012r2-std"
    )
  )

  $subnet = & yc vpc subnet get $subnet_name --format json | ConvertFrom-Json
  $zone_id = $subnet.zone_id

  foreach ($prefix in $prefixes) {
    $i = $prefix + "-" + $type + "-" + $date

    & yc compute instance create `
      --name $i `
      --zone $zone_id `
      --memory $memory_gb `
      --cores $cores `
      --metadata-from-file user-data=$userdata_path `
      --create-boot-disk `
        type=network-nvme,image-name=$i `
      --network-interface `
        subnet-name=$subnet_name,nat-ip-version=ipv4 `
      --async
  }
}

$common = @{ 
  userdata_path = 'C:\Users\mryzh\Desktop\YC\_mkt\fabrique\setpassnew'
  type = "base"
  date = "200824-1012"
}

Create-YCBaseInstances @common
```

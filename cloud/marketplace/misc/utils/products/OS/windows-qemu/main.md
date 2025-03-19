# HowTo build windows from scratch

* Create instance @ preprod, any way you prefer, c2-m6-d50+ is ok. JFYIO There is test-build-instance with nested virtualization enabled @ preprod.
* Download binaries. You could use eval and then switch to kms, but one of links is http, so it is better to use VL portal.
* Download drivers. https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/ - latest stable.
* Place drivers, iso's and their sha1 (must end with `\n`) @ corresponding folders.
* Run `packer build -var name=ws19 windows.iso.packer.json` changing 'name' accordingly, two images std and dc will be produced
* Look @ process with `ssh -L [local_port]:localhost:[vnc_port] [remote_it]`
* Upload 2 win-distr using aws-cli
* Create images from s3 (cli won't propagate os_type) using presigned uri's, helper with example below

```(powershell)
function Create-YCImage {
  param(
    [ValidateNotNullOrEmpty()]
    [string]$folderId = "",

    [ValidateNotNullOrEmpty()]
    [string]$name = "",

    [string]$description = "",

    [ValidateNotNullOrEmpty()]
    [string]$os_type = "WINDOWS",

    [int64]$minDiskSizeGb = 50GB,

    [ValidateNotNullOrEmpty()]
    [string]$uri = ""
  )

  $body = @"
{
  "folderId": "$folderId",
  "name": "$name",
  "description": "$description",
  "os.type": "$os_type",
  "minDiskSize": "$minDiskSizeGb",
  "os": {
    "type": "$os_type"
  },
  "uri": "$uri"
}
"@

  Invoke-WebRequest `
    -Method POST `
    -URI https://compute.api.cloud.yandex.net/compute/v1/images `
    -header @{ "Authorization" = "Bearer $(& yc iam create-token)" } `
    -ContentType 'Application/json' `
    -body $body
}

$folderId = ""
$windows_2012r2_std_gvlk_qemu_uri = ""

Create-YCImage `
  -folderId $folderId `
  -name "windows-2012r2-std-gvlk-qemu" `
  -family "windows-2012r2-std-gvlk" `
  -uri $windows_2012r2_std_gvlk_qemu_uri

```

* you can create instances and check'em up, helper with example below

```
# active yc profile will be used
function Create-YCInstance {
  param(
    [ValidateNotNullOrEmpty()]
    [string]$name = "",
    
    [ValidateNotNullOrEmpty()]
    [string]$userdata_path = "",

    [ValidateRange(0,16)]
    [int]$memory_gb = 4,
    
    [ValidateRange(0,16)]
    [int]$cores = 2,
    
    [ValidateNotNullOrEmpty()]
    [string]$image_id = "",
    
    [ValidateNotNullOrEmpty()]
    [string]$subnet_name = ""
  )

  $subnet = & yc vpc subnet get $subnet_name --format json | ConvertFrom-Json
  $zone_id = $subnet.zone_id

  & yc compute instance create `
    --name $name `
    --zone $zone_id `
    --memory $memory_gb `
    --cores $cores `
    --metadata-from-file user-data=$userdata_path `
    --create-boot-disk `
      type=network-nvme,image-id=$image_id `
    --network-interface `
      subnet-name=$subnet_name,nat-ip-version=ipv4 `
    --async
}

$userdata = "& net user Administrator MakeQWErty$trongAga1n"

$subnet_name = ""
$windows_2019_std_image_id = ""
<...>
$windows_2012r2_std_image_id = ""
$userdata_path = 'C:\Users\mryzh\Desktop\YC\_mkt\fabrique\setpassnew'
$userdata | Out-File $userdata_path # ugly but i dont care

# common params swipe under splatting
$common = @{ userdata_path = $userdata_path; subnet_name = $subnet_name }

# we can create'em and check

Create-YCInstance -name ws19std -image_id $windows_2019_std_image_id @common

<...>

Create-YCInstance -name ws12r2dc -image_id $windows_2012r2_std_image_id @common
```

* if you built only std or eval editions - you should promote'em interactively via PowerShell-Remoting + Packer (recipie 'windows.yc.packer.json'), helper with example below

```(powershell)
function Set-WindowsEdition {
  param(
    [Parameter(Mandatory)]  
    [ValidateSet(
      'Windows Server 2012 R2 Datacenter',
      'Windows Server 2016 Datacenter',
      'Windows Server 2019 Datacenter',
      'Windows Server 2012 R2 Standard',
      'Windows Server 2016 Standard',
      'Windows Server 2019 Standard'
    )]
    [string]
    $ProductName
  )

  switch ($ProductName) {
    # well-known keys
    'Windows Server 2012 R2 Datacenter' {
      $KMSClientkey = 'W3GGN-FT8W3-Y4M27-J84CP-Q3VJ9'
    }
    'Windows Server 2016 Datacenter' {
      $KMSClientkey = 'CB7KF-BWN84-R7R2Y-793K2-8XDDG'
    }
    'Windows Server 2019 Datacenter' {
      $KMSClientkey = 'WMDGN-G9PQG-XVVXX-R3X43-63DFG'
    }
    'Windows Server 2012 R2 Standard' {
      $KMSClientkey = 'D2N9P-3P6X9-2R39C-7RTCD-MDVJX'
    }
    'Windows Server 2016 Standard' {
      $KMSClientkey = 'WC2BQ-8NRM3-FDDYY-2BFGV-KHKQY'
    }
    'Windows Server 2019 Standard' {
      $KMSClientkey = 'N69G4-B89J2-4G8F4-WWYCC-J464C'
    }
  }

  if ( $ProductName -match "Datacenter" ) {
    $edition = "ServerDatacenter"
  } else {
    $edition = "ServerStandard"
  }

  & dism /online /Set-Edition:$edition /ProductKey:$KMSClientkey /AcceptEula
}

$c = Get-Credential -UserName Administrator
$so = New-PSSessionOption -SkipCACheck -SkipRevocationCheck -SkipCNCheck
$ip = 1.1.1.1

Enter-PSSession -ComputerName $ip -UseSSL -Credential $c -SessionOption $so

```

This is very basic images, instances will execute any script in 'user-data' without any agent inside using task scheduler.

## Notes

* In autounattend.xml 'Image index' = 4 stands for DataCenter, 2 for Standard, odds for corresponding core edition's. So, frankly speaking - each image contain std & dc, you could build only std coz quemu builder is slow as molasses and then convert'em.
* Binaries could be found @ MSFT VL portal or s3 bucket 'win-distr'

# ToDo:
#      * add versioning to 'windows-scripts'
#        since packer cant pass parameter to powershell_script provisioner
#        we'll do it a bit l8r and now will hard-code on per recipie basis =\


param(
  [string]$WINDOWS_SCRIPTS_VERSION = "3"
)

$BOOTSTRAP_FOLDER       = "C:\bootstrap"
$WINDOWS_SCRIPTS_URI    = "https://storage.yandexcloud.net/win-distr/windows-scripts-v$($WINDOWS_SCRIPTS_VERSION).zip"
$WINDOWS_SCRIPTS_DST    = "$BOOTSTRAP_FOLDER\windows-scripts.zip"
$WINDOWS_SCRIPTS_FOLDER = "$BOOTSTRAP_FOLDER\windows-scripts"
$PS51_URI               = "https://storage.yandexcloud.net/Win8.1AndW2K12R2-KB3191564-x64.msu"
$PS51_DST               = "$BOOTSTRAP_FOLDER\PSV51.msu"
$SETUPCOMPLETE_FOLDER   = "C:\Windows\Setup\Scripts"

function Get-S3File {
  param(
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    $Uri,

    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    $OutFile
  )

  # looking for AWS_SECRET_ACCESS_KEY & AWS_ACCESS_KEY_ID in ENV
  $HMACSHA1 = [System.Security.Cryptography.KeyedHashAlgorithm]::Create("HMACSHA1")
  $HMACSHA1.Key = [System.Text.Encoding]::UTF8.Getbytes($env:AWS_SECRET_ACCESS_KEY)
  $date = (Get-Date).ToUniversalTime().ToString("ddd, dd MMM yyyy HH:mm:ss 'GMT'")
  $subUri = $Uri -replace 'https://storage.yandexcloud.net', ''
  $stringToSign = [System.Text.Encoding]::UTF8.Getbytes("GET`n`napplication/x-compressed-zip`n$date`n$subUri")
  $signedString = [Convert]::Tobase64String($HMACSHA1.ComputeHash($stringToSign))

  # workaround for slow iwr download
  $pp = $ProgressPreference
  $ProgressPreference = 'SilentlyContinue'

  $r = @{
    Method = "Get"
    Uri = $Uri
    OutFile = $OutFile
    Header = @{
      Host = "storage.yandexcloud.net"
      "Date"          = $date
      "Content-Type"  = "application/x-compressed-zip"
      "Authorization" = "AWS $($env:AWS_ACCESS_KEY_ID):$($signedString)"
    }
  }

  Invoke-RestMethod @r
  $ProgressPreference = $pp
}

##################################################

# remove if there any leftovers (dirty quemu build)
if (Test-Path $SETUPCOMPLETE_FOLDER) { rm $SETUPCOMPLETE_FOLDER -Confirm:$false -Recurse }
mkdir $SETUPCOMPLETE_FOLDER | out-null

# remove if there any leftovers (previous builds in pipeline)
if (Test-Path $BOOTSTRAP_FOLDER) { rm $BOOTSTRAP_FOLDER -Confirm:$false -Recurse }
mkdir $BOOTSTRAP_FOLDER | out-null

# download 'windows-scripts'
$ws = @{
  Uri = $WINDOWS_SCRIPTS_URI
  OutFile = $WINDOWS_SCRIPTS_DST
}
Get-S3File @ws

# PSv4 compatible, hello ws12r2
Add-Type -assembly "system.io.compression.filesystem"
[io.compression.zipfile]::ExtractToDirectory($WINDOWS_SCRIPTS_DST, $WINDOWS_SCRIPTS_FOLDER)

# ws12r2 again
if ($PSVersionTable.PSVersion.Major -lt 5) {
  $ps = @{
    Uri = $PS51_URI
    OutFile = $PS51_DST
  }
  Get-S3File @ps

  Start-Process -FilePath "$env:SystemRoot\system32\wusa.exe" -ArgumentList "$PS51_DST /quiet /norestart" -Wait
}

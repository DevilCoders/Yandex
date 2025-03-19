# create dir
$BOOTSTRAP_FOLDER = "c:\bootstrap"
if (Test-Path $BOOTSTRAP_FOLDER) { rm $BOOTSTRAP_FOLDER -Confirm:$false -Recurse }
mkdir $BOOTSTRAP_FOLDER | out-null

# signing magic
$WINDOWS_SCRIPTS_S3 = "/win-distr/windows-scripts.zip"
$HMACSHA1 = [System.Security.Cryptography.KeyedHashAlgorithm]::Create("HMACSHA1")
$HMACSHA1.Key = [System.Text.Encoding]::UTF8.Getbytes($env:AWS_SECRET_ACCESS_KEY)
$date = (Get-Date).ToUniversalTime().ToString("ddd, dd MMM yyyy HH:mm:ss 'GMT'")
$stringToSign = [System.Text.Encoding]::UTF8.Getbytes("GET`n`napplication/x-compressed-zip`n$date`n$WINDOWS_SCRIPTS_S3")
$signedString = [Convert]::Tobase64String($HMACSHA1.ComputeHash($stringToSign))

$ProgressPreference = 'SilentlyContinue' # workaround for slow iwr download
Invoke-RestMethod `
  -Method Get `
  -Uri "https://storage.yandexcloud.net/win-distr/windows-scripts.zip" `
  -OutFile "$BOOTSTRAP_FOLDER\windows-scripts.zip" `
  -Header @{
      "Host"          = "storage.yandexcloud.net"
      "Date"          = $date
      "Content-Type"  = "application/x-compressed-zip"
      "Authorization" = "AWS $($env:AWS_ACCESS_KEY_ID):$($signedString)"
    }

# PSv4 compatible, hello ws12r2
Add-Type -assembly "system.io.compression.filesystem"
[io.compression.zipfile]::ExtractToDirectory("$BOOTSTRAP_FOLDER\windows-scripts.zip", "$BOOTSTRAP_FOLDER\windows-scripts")

# ws12r2 again
if ($PSVersionTable.PSVersion.Major -lt 5) {
  #l8r place to s3
  Invoke-RestMethod `
    -Method Get `
    -Uri "https://download.microsoft.com/download/6/F/5/6F5FF66C-6775-42B0-86C4-47D41F2DA187/Win8.1AndW2K12R2-KB3191564-x64.msu" `
    -OutFile "$BOOTSTRAP_FOLDER\PSV51.msu"

  Start-Process -FilePath "$env:SystemRoot\system32\wusa.exe" -ArgumentList "$BOOTSTRAP_FOLDER\PSV51.msu /quiet /norestart" -Wait
}

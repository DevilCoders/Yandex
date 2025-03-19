# create dir
$BOOTSTRAP_FOLDER = "c:\bootstrap"
if (Test-Path $BOOTSTRAP_FOLDER) { rm $BOOTSTRAP_FOLDER -Confirm:$false -Recurse }
mkdir $BOOTSTRAP_FOLDER | out-null

# signing magic
$HMACSHA1 = [System.Security.Cryptography.KeyedHashAlgorithm]::Create("HMACSHA1")
$HMACSHA1.Key = [System.Text.Encoding]::UTF8.Getbytes($env:AWS_SECRET_ACCESS_KEY)
$date = (Get-Date).ToUniversalTime().ToString("ddd, dd MMM yyyy HH:mm:ss 'GMT'")
$stringToSign = [System.Text.Encoding]::UTF8.Getbytes("GET`n`napplication/x-compressed-zip`n$date`n/win-distr/windows-scripts.zip")
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

cd $BOOTSTRAP_FOLDER
Expand-Archive .\windows-scripts.zip

# setupcomplete preparations
$SETUPCOMPLETE_FOLDER = "C:\Windows\Setup\Scripts"
if ( -not (Test-Path $SETUPCOMPLETE_FOLDER) ) { mkdir $SETUPCOMPLETE_FOLDER }
#cp $BOOTSTRAP_FOLDER\unattend\setupcomplete.cmd $SETUPCOMPLETE_FOLDER
#cp $BOOTSTRAP_FOLDER\unattend\sysprepunattend.xml $SETUPCOMPLETE_FOLDER
#cp $BOOTSTRAP_FOLDER\ensure-setupcomplete.ps1 $SETUPCOMPLETE_FOLDER
#cp $BOOTSTRAP_FOLDER\ensure-setupcompletecleanup.ps1 $SETUPCOMPLETE_FOLDER

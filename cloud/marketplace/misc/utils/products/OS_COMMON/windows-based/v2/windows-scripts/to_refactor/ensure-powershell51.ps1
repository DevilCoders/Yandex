# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-PowerShell51 { 
  if ( ($PSVersionTable.PSVersion.Major -lt 5) -and ((Get-WindowsVersion) -match "Windows Server 2012 R2 *") ) {
    # l8r pull to our s3
    $url = 'https://download.microsoft.com/download/6/F/5/6F5FF66C-6775-42B0-86C4-47D41F2DA187/Win8.1AndW2K12R2-KB3191564-x64.msu'
    $KnownFileHash = '91D95A0CA035587D4C1BABE491F51E06A1529843'
    
    Start-BitsTransfer -Source $url -Destination $Env:TEMP\PSV51.msu

    $DownloadedFileHash = Get-FileHash -Path $Env:TEMP\PSV51.msu -Algorithm:SHA1 | Select-Object -ExpandProperty Hash
    if ($KnownFileHash -eq $DownloadedFileHash) {
      Start-Process -FilePath 'wusa.exe' -ArgumentList "$Env:TEMP\PSV51.msu /quiet /norestart" -Wait
      Remove-Item -Path "$Env:TEMP\PSV51.msu"
    } else {
      # Log/Crit if needed?
    }
  } else {
    # nice!
  }
}


function Test-PowerShell51 {

}

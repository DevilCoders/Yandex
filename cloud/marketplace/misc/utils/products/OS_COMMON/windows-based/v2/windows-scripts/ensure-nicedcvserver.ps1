# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-globals.ps1"
. "$PSScriptRoot\common\yandex-powershell-s3.ps1"


function Ensure-NICEDCVServer {
  "[ENFORCE]::Ensure NICEDCVServer" | Out-InfoMessage
  
  "[ENFORCE]::Installing" | Out-InfoMessage
  $i = @{
    FilePath = "msiexec.exe"
    Args = "/i $(ls c:\distr\nicedcvserver -filter *.msi | select -exp fullname) /quiet /norestart"
  }
  Start-Process -PassThru @i | Wait-Process
  
  return (Test-NICEDCVServer)
}


function Test-NICEDCVServer {
  param(
    $display_name = "NICE Desktop Cloud Visualization Server*"
  )

  $r = (Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* | 
    ? DisplayName -like "*$display_name*" | Select -First 1) -ne $null
  "[TEST]::$display_name installed? $r" | Out-InfoMessage

  return $r
}

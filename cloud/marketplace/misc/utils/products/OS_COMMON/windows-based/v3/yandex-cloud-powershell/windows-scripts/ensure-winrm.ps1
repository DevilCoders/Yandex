# ToDo:
#       * Change from removing all certificates to removing certificates without pk


# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"
. "$PSScriptRoot\common\yandex-powershell-common.ps1"


function Ensure-Winrm {
  "[ENFORCE]::Ensure WINRM" | Out-InfoMessage

  Clear-WinrmListeners
  Clear-MyCertificates
  New-WinrmHTTPListener
  New-WinrmHTTPSListener -CertificateThumbPrint (New-WinrmCertificate).ThumbPrint
  Enable-WinrmFirewallRules
  
  return (Test-Winrm)
}


function Test-Winrm {
  # http listener
  $http_listener = -not [string]::IsNullOrEmpty( (Get-WinrmHTTPListener) )
  "[TEST]::HTTP listener exists? $http_listener" | Out-InfoMessage
  
  # https listener
  $https_listener = -not [string]::IsNullOrEmpty( (Get-WinrmHTTPSListener) )
  "[TEST]::HTTPS listener exists? $https_listener" | Out-InfoMessage

  # certificate with correct name and pk
  $cert_tp = ls "WSMan:\localhost\Listener\$(Get-WinrmHTTPSListener)\CertificateThumbPrint" | `
    Select-Object -ExpandProperty Value
  $cert = ls Cert:\LocalMachine\My\$cert_tp -ea:SilentlyContinue
  "[TEST]::Certificate exists? $($cert -ne $null)" | Out-InfoMessage

  $cert_name = ($cert.DnsNameList.Unicode).toLower() -contains ([System.Net.Dns]::GetHostByName($env:computerName).Hostname).toLower()
  if ($WindowsVersion -notmatch "Windows Server 2012 R2*") {
    # ws12 needs only dnsName property, others require subject in addition
    $cert_name = $cert_name -and ( ($cert.Subject -replace 'CN=').toLower() -eq ($env:computerName).toLower() )
  }
  "[TEST]::Certificate name(s) properties correct? $cert_name" | Out-InfoMessage
  
  $cert_pk = $cert.HasPrivateKey -ne $null
  "[TEST]::Certificate private key exists? $cert_pk" | Out-InfoMessage

  # firewall rules, .enabled is sting -_-
  $fw_http = (Get-NetFirewallRule -Name "WINRM-HTTP-In-TCP" -ErrorAction SilentlyContinue).Enabled -ne "False"
  "[TEST]::https listener exists? $cert_pk" | Out-InfoMessage

  $fw_https = (Get-NetFirewallRule -Name "WINRM-HTTPS-In-TCP" -ErrorAction SilentlyContinue).Enabled -ne "False"
  "[TEST]::https listener exists? $cert_pk" | Out-InfoMessage

  return ($http_listener -and `
          $https_listener -and `
          ($cert -ne $null) -and `
          $cert_name -and `
          $cert_pk -and `
          $fw_http -and `
          $fw_https)
}

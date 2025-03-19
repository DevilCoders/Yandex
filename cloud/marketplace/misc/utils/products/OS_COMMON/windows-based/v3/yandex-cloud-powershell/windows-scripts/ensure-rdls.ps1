# ToDo/Note
#           * since we'll make one-click-deployment in future
#             exposing eglible lic count is not needed


# include
. "$PSScriptRoot\common\yandex-powershell-logging.ps1"


function Ensure-RDLS {
  param(
    [Parameter(Mandatory=$true)]
    [ValidateSet(
      5,
      10,
      25,
      50,
      100,
      250,
      500)]
    [int]$Count,

    [Parameter(Mandatory=$true)]
    [ValidateRange(1, [int]::MaxValue)]
    [int]$Agreement
  )

  "[ENFORCE]::Ensure RDLS" | Out-InfoMessage
  "[ENFORCE]::Installing RDLS role" | Out-InfoMessage
  Install-WindowsFeature -Name RDS-Licensing -IncludeManagementTools | out-Null
  Import-Module RemoteDesktopServices

  # Activate
  "[ENFORCE]::Activating server" | Out-InfoMessage
  Set-Item -Path 'RDS:\LicenseServer\Configuration\FirstName'     -Value 'Yandex'
  Set-Item -Path 'RDS:\LicenseServer\Configuration\LastName'      -Value 'Cloud'
  Set-Item -Path 'RDS:\LicenseServer\Configuration\Company'       -Value 'Yandex Cloud'
  Set-Item -Path 'RDS:\LicenseServer\Configuration\CountryRegion' -Value 'Russia'
  Set-Item -Path 'RDS:\LicenseServer\ActivationStatus' `
    -Value "1" `
    -ConnectionMethod "AUTO" `
    -Reason "5"

  # Install Licenses
  "[ENFORCE]::Install Licenses" | Out-InfoMessage
  $AgreementType   = 4 # 4 is SPLA
  $ProductVersion  = 6 # 6 is 2019
  $LicenseCount    = $Count
  $ProductType     = 1 # 1 is PerUser
  $AgreementNumber = $Agreement

  # PSDrive method of installing licenses lacs
  $r = Invoke-CimMethod -ClassName 'Win32_TSLicenseKeyPack' -MethodName InstallAgreementLicenseKeyPack `
    @{
        AgreementType    = [UInt32]$AgreementType;
        LicenseCount     = [UInt32]$LicenseCount;
        ProductType      = [UInt32]$ProductType;
        ProductVersion   = [UInt32]$ProductVersion;
        sAgreementNumber = [string]$AgreementNumber;
    }
  
  return (Test-RDLS)
}


function Test-RDLS {
  Import-Module RemoteDesktopServices -ea:SilentlyContinue
  
  # activaded
  $a = (ls RDS:\LicenseServer\ActivationStatus -ea:SilentlyContinue).CurrentValue -eq 1
  "[TEST]::Server is activaded? $a" | Out-InfoMessage

  # contains lic pack with expected count of licenses
  if ($t = (ls RDS:\LicenseServer\LicenseKeyPacks\*\TotalLicenses -ea:SilentlyContinue).CurrentValue | ? {$_ -ne '4294967295'}) {
    $d = (diff `
              @( 5, 10, 25, 50, 100, 250, 500 ) `
              $t `
              -IncludeEqual `
              -ea:SilentlyContinue).SideIndicator
  }
  $c = $d -contains "=="
  "[TEST]::Server contains license pack with allowed number of licenses? $c" | Out-InfoMessage
  
  return ($a -and $c -and $t)
}

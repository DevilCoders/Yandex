# importing all libraries from common directory ( gci "$PSScriptRoot\common" -fi *.ps1 | select -exp FullName | % { . $_ }; )
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1
foreach ($lib in $libs.FullName) { . $lib }

# expose
function ensure-rdls ($lic_num, $spla_num) {
  Write-Host "Installing RDLS role"
  Install-WindowsFeature -Name RDS-Licensing -IncludeManagementTools
  Import-Module RemoteDesktopServices

  # Activate
  Write-Host "Activating server"
  Set-Item -Path 'RDS:\LicenseServer\Configuration\FirstName'     -Value 'Yandex'
  Set-Item -Path 'RDS:\LicenseServer\Configuration\LastName'      -Value 'Cloud'
  Set-Item -Path 'RDS:\LicenseServer\Configuration\Company'       -Value 'Yandex Cloud'
  Set-Item -Path 'RDS:\LicenseServer\Configuration\CountryRegion' -Value 'Russia'
  Set-Item -Path 'RDS:\LicenseServer\ActivationStatus' `
    -Value "1" `
    -ConnectionMethod "AUTO" `
    -Reason "5"

  # Install Licenses
  Write-Host "Install Licenses"
  $AgreementType       = 4 # 4 is SPLA
  $SPLAAgreementNumber = $spla_num
  $ProductVersion      = 6 # 6 is 2019
  $ProductType         = 1 # 1 is PerUser
  $LicenseCount        = $lic_num

  # PSDrive method of installing licenses lacs
  Invoke-CimMethod -ClassName 'Win32_TSLicenseKeyPack' -MethodName InstallAgreementLicenseKeyPack `
    @{
        AgreementType    = [UInt32]$AgreementType;
        LicenseCount     = [UInt32]$LicenseCount;
        ProductType      = [UInt32]$ProductType;
        ProductVersion   = [UInt32]$ProductVersion;
        sAgreementNumber = [string]$SPLAAgreementNumber;
    }
}

# runnable
if (-not (Test-DotSourced)) { ensure-rdls $args[0] $args[1] }

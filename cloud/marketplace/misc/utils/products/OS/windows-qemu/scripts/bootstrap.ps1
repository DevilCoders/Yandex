# shutdown without active session
Set-ItemProperty `
  -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System" `
  -Name "ShutdownWithoutLogon" `
  -Value 1

# invoke userdata at startup
& schtasks /Create /TN "userdata" /RU SYSTEM /SC ONSTART /RL HIGHEST /TR "Powershell -NoProfile -ExecutionPolicy Bypass -Command \`"& {iex (irm -H @{\\\`"Metadata-Flavor\\\`"=\\\`"Google\\\`"} \\\`"http://169.254.169.254/computeMetadata/v1/instance/attributes/user-data\\\`")}\`"" | Out-Null

# setupcomplete
mkdir "C:\Windows\Setup\Scripts" | Out-Null
cp "$PSScriptRoot\setupcomplete.cmd" "C:\Windows\Setup\Scripts\"
cp "$PSScriptRoot\setupcomplete.ps1" "C:\Windows\Setup\Scripts\"

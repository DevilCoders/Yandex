ls "$PSScriptRoot\common" -fil *.ps1 | % { . $_.FullName }

# expose
function ensure-userdata { 
  # run script from metadata
  & schtasks /Create /TN "userdata" /RU SYSTEM /SC ONSTART /RL HIGHEST /TR "Powershell -NoProfile -ExecutionPolicy Bypass -Command \`"& {iex (irm -H @{\\\`"Metadata-Flavor\\\`"=\\\`"Google\\\`"} \\\`"http://169.254.169.254/computeMetadata/v1/instance/attributes/user-data\\\`")}\`""
}

# runnable
if (-not (Test-DotSourced)) { ensure-userdata }

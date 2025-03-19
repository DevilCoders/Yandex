# importing all libraries from common directory ( gci "$PSScriptRoot\common" -fi *.ps1 | select -exp FullName | % { . $_ }; )
$libs = Get-ChildItem "$PSScriptRoot\common" -Filter *.ps1 | Select-Object -ExpandProperty FullName
foreach ($lib in $libs.FullName) { . $lib }

# expose
function ensure-sqlservermanagementstudio { 
  
}

# runnable
if (-not (Test-DotSourced)) { ensure-sqlservermanagementstudio }

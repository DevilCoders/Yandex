# expose
function ensure-boilerplate { 
  
}

# runnable
if (-not (Test-DotSourced)) {
  ls "$PSScriptRoot\common" -fil *.ps1 | % { . $_.FullName }
  ensure-boilerplate
}

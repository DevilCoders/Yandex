$PowerShellScripts = Get-ChildItem "C:\Windows\Setup\Scripts" -Filter "*.ps1"
foreach ($ScriptName in $PowerShellScripts.Name) {
    & $PSScriptRoot\$ScriptName
}

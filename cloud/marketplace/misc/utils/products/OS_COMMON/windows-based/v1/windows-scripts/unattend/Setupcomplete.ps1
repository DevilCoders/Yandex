ls "C:\Windows\Setup\Scripts" -fil *.ps1 | ? name -ne "Setupcomplete.ps1" | % { & $_.FullName }
ls "C:\Windows\Setup\Scripts" -rec       | ? name -ne "Setupcomplete.cmd" | rm -rec -confirm:$false

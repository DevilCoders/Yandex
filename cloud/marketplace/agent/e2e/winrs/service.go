package winrs

import (
	"fmt"

	"github.com/masterzen/winrm"
)

func (w *WinRS) StartService(name string) error {
	return w.runCommand(winrm.Powershell(fmt.Sprintf(`Start-Service -Name %s`, name)))
}

func (w *WinRS) GetService(name string) error {
	return w.runCommandSilently(winrm.Powershell(fmt.Sprintf(`Get-Service -Name '%v'`, name)))
}

func (w *WinRS) WaitServiceRunning(name string) error {
	c := fmt.Sprintf(`
		$ServiceName = '%v'
		$Timeout = 30
		$Deadline = ([datetime]::Now).AddSeconds($timeout)

		do {
			$Status = (Get-Service -Name $ServiceName -ea:SilentlyContinue).Status
			if ($Status -eq 'Running') {
				Exit 0
			}

			"Waiting service: $ServiceName to become 'Running', currently $Status" | Out-Host
			Start-Sleep -Seconds 1
		} while ( ($Status -ne "Running") -and ([datetime]::Now -lt $Deadline) )

		Exit 1
	`, name)

	return w.runCommand(winrm.Powershell(c))
}

func (w *WinRS) StopService(name string) error {
	return w.runCommand(winrm.Powershell(fmt.Sprintf(`Stop-Service -Name '%v'`, name)))
}

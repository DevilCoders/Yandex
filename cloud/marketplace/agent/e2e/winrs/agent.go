package winrs

import (
	"fmt"

	"github.com/masterzen/winrm"
)

func (w *WinRS) GuestAgentInstall(path string) error {
	c := fmt.Sprintf(`
		$path = '%v'
		& $path install
		Exit 0`, path)

	return w.runCommand(winrm.Powershell(c))
}

func (w *WinRS) WaitGuestAgentVersionToBecome(path, version string) error {
	c := fmt.Sprintf(`
		$Path = '%v'
		$WantVersion = '%v'

		$Timeout = 30
		$Deadline = ([datetime]::Now).AddSeconds($timeout)

		do {
			$GotVersion = & $Path version
			if ($GotVersion -eq $WantVersion) {
				"Good, good, got: $GotVersion (that's one we wanted!)" | Out-Host
				Exit 0
			}

			"Polling guest agent version, currently: $GotVersion, waiting for $WantVersion" | Out-Host
			Start-Sleep -Seconds 1
		} while ( [datetime]::Now -lt $Deadline )

		Exit 1
		`, path, version)

	return w.runCommand(winrm.Powershell(c))
}

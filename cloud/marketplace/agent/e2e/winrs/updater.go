package winrs

import (
	"fmt"

	"github.com/masterzen/winrm"
)

func (w *WinRS) GuestAgentUpdaterRemove(path string) error {
	c := fmt.Sprintf(`
		$path = '%v'
		& $path remove
		Exit 0`, path)

	return w.runCommand(winrm.Powershell(c))
}

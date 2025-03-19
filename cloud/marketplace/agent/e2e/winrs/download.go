package winrs

import (
	"fmt"

	"github.com/masterzen/winrm"
)

func (w *WinRS) Download(url, out string) error {
	c := fmt.Sprintf(`
		Invoke-RestMethod '%v' -OutFile '%v'
		if ($?) { Exit 0 } else { Exit 1 }`,
		url, out)

	return w.runCommand(winrm.Powershell(c))
}

package winrs

import (
	"fmt"

	"github.com/masterzen/winrm"
)

func (w *WinRS) RemoveRegKey(path string) error {
	c := fmt.Sprintf(`Get-Item -Path %s -ea:SilentlyContinue | Remove-Item -ea:SilentlyContinue; Exit 0`, path)

	return w.runCommand(winrm.Powershell(c))
}

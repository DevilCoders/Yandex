package winrs

import (
	"fmt"
	"strings"

	"github.com/masterzen/winrm"
)

func (w *WinRS) RemoveDir(path string) error {
	c := fmt.Sprintf(`
		Remove-Item -Path '%v' -Recurse -Force -Confirm:$false -ea:SilentlyContinue; Exit 0
		`, path)

	return w.runCommand(winrm.Powershell(c))
}

func (w *WinRS) CreateDirAll(path string) error {
	c := fmt.Sprintf(`
		New-Item -Path '%v' -ItemType:Directory -Force -Confirm:$false | Out-Null; Exit 0
		`, path)

	return w.runCommand(winrm.Powershell(c))
}

func (w *WinRS) GetFileSha256(path string) (string, error) {
	c := fmt.Sprintf(`
		$h = Get-FileHash '%v' -Algorithm:SHA256 -ErrorAction:SilentlyContinue
 		if ($h -ne $null) {
			$h.Hash.ToLower()
		}
		Exit 0
		`, path)

	h, err := w.runCommandS(winrm.Powershell(c))
	if err != nil {
		return "", err
	}

	return strings.ReplaceAll(h, "\r\n", ""), nil
}

func (w *WinRS) RemoteCopy(dst, src string) error {
	c := fmt.Sprintf(`
		Copy-Item -Destination '%v' -Path '%v' -Force -ErrorAction:Stop;Exit 0
		`, dst, src)

	return w.runCommand(winrm.Powershell(c))
}

func (w *WinRS) TestFile(path string) error {
	c := fmt.Sprintf(`
		if (Test-Path '%v') {
			Exit 0
		} else {
			Exit 1
		}
		`, path)

	return w.runCommand(winrm.Powershell(c))
}

func (w *WinRS) GetContent(path string) error {
	c := fmt.Sprintf(`
		$Log = Get-Content '%v'
		foreach($Line in $Log) {
			"TEST LOG FROM INSTANCE: $Line" | Out-Default
		}
		`, path)

	return w.runCommand(winrm.Powershell(c))
}

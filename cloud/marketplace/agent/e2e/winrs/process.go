package winrs

import (
	"fmt"
	"strconv"
	"strings"

	"github.com/masterzen/winrm"
)

func (w *WinRS) StopProcess(pid int) error {
	return w.runCommand(winrm.Powershell(fmt.Sprintf("Stop-Process -Id %s", strconv.Itoa(pid))))
}

func (w *WinRS) CheckProcess(pid int) error {
	return w.runCommand(winrm.Powershell(fmt.Sprintf("Get-Process -Id %s | Out-Null", strconv.Itoa(pid))))
}

func (w *WinRS) GetProcessID(name string) (pid int, err error) {
	// will throw error is there two processess with same name
	c := fmt.Sprintf(`[int](Get-Process -Name "%s" | Select-Object -ExpandProperty Id)`, name)
	var stdout string
	stdout, err = w.runCommandS(winrm.Powershell(c))
	if err != nil {
		return
	}

	pid, err = strconv.Atoi(strings.TrimRight(stdout, "\r\n"))

	return
}

func (w *WinRS) StartProcess(path string, args ...string) (pid int, err error) {
	a := strings.Join(args[:], " ")
	c := fmt.Sprintf(
		`([WMICLASS]"\\localhost\ROOT\CIMV2:win32_process").Create("%s %s") | Select-Object -ExpandProperty ProcessId`,
		path, a)

	var stdout string
	stdout, err = w.runCommandS(winrm.Powershell(c))
	if err != nil {
		return
	}

	pid, err = strconv.Atoi(strings.TrimRight(stdout, "\r\n"))

	return
}

// sendCtrlC sends CtrlC signal to process with name `Agent`
func (w *WinRS) StartProcessAndStopByCtrlC(path string) error {
	c := fmt.Sprintf(`
		$startAgent = New-ScheduledTaskAction -Execute "%s" -Argument "update --log-level debug"
		Register-ScheduledTask -TaskName "startAgent" -Action $startAgent -User "System" | Out-Null
		Start-ScheduledTask startAgent
		Start-Sleep -Seconds 30
		Stop-ScheduledTask startAgent
		Unregister-ScheduledTask startAgent -Confirm:$false`, path)

	return w.runCommand(winrm.Powershell(c))
}

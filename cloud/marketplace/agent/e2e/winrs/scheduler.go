package winrs

import (
	"fmt"

	"github.com/masterzen/winrm"
)

func (w *WinRS) CreateTaskSchedulerJob(path, arg, name string) error {
	c := fmt.Sprintf(`
		$Path = '%v'
		$Arg = '%v'
		$Name = '%v'

		$Action = New-ScheduledTaskAction -Execute $path -Argument $arg

		Register-ScheduledTask -TaskName $Name -Action $Action -User "System" -RunLevel:Highest | Out-Null
		`, path, arg, name)

	return w.runCommand(winrm.Powershell(c))
}

func (w *WinRS) RemoveTaskSchedulerJob(name string) error {
	c := fmt.Sprintf(`
		$name = '%v'

		Stop-ScheduledTask -TaskName $name
		Unregister-ScheduledTask -TaskName $name -Confirm:$false
		`, name)

	return w.runCommand(winrm.Powershell(c))
}

func (w *WinRS) StartTaskSchedulerJob(name string) error {
	c := fmt.Sprintf(`
		$Name = '%v'

		$SchedSvc = New-Object -ComObject "Schedule.Service"
		$SchedSvc.Connect()
		$SchedFolder = $SchedSvc.GetFolder("\")
		$SchedTask = $SchedFolder.GetTask($Name)
		$SchedTask.Run($null) | Out-Null

		$Timeout = 30
		$Deadline = ([datetime]::Now).AddSeconds($timeout)

		do {
			"Waiting task to finish" | Out-Host
			Start-Sleep -Seconds 1
		} while ( (-not ($SchedTask.State -eq 4)) -and ([datetime]::Now -lt $Deadline) )
		`, name)

	return w.runCommand(winrm.Powershell(c))
}

package winrs

import (
	"fmt"
	"strconv"
	"strings"

	"github.com/masterzen/winrm"
)

func (w *WinRS) AddLocalGroupMember(group, user string) error {
	c := fmt.Sprintf(`Add-LocalGroupMember -Member "%s" -Name "%s"`, user, group)

	return w.runCommand(winrm.Powershell(c))
}

func (w *WinRS) CheckMemberOfAdministrators(user string) (ok bool, err error) {
	c := fmt.Sprintf(`
		# S-1-5-32-544 is well-known SID
		$Members = Get-LocalGroup -SID S-1-5-32-544 |
			Get-LocalGroupMember |
				Select-Object -ExpandProperty SID |
					Select-Object -ExpandProperty Value

		$User = Get-LocalUser -Name "%v" |
			Select-Object -ExpandProperty SID |
				Select-Object -ExpandProperty Value

		$Members -contains $User
		`, user)

	var stdout string
	stdout, err = w.runCommandS(winrm.Powershell(c))
	if err != nil {
		return
	}

	ok, err = strconv.ParseBool(strings.TrimRight(stdout, "\r\n"))

	return
}

func (w *WinRS) CreateAdministrator(user, pass string) error {
	c := fmt.Sprintf(`
		$MyUserName = "%v"
		$MyPlainTextPassword = "%v"
		$MyPassword = $MyPlainTextPassword | ConvertTo-SecureString -AsPlainText -Force
		$MyUser = New-LocalUser -Name $MyUserName -Password $MyPassword -PasswordNeverExpires -AccountNeverExpires
		$MyUser | Add-LocalGroupMember -Group 'Administrators'
		$MyUser | Add-LocalGroupMember -Group 'Remote Management Users'
		`, user, pass)

	return w.runCommand(winrm.Powershell(c))
}

func (w *WinRS) RemoveUser(user string) error {
	c := fmt.Sprintf(`
		Get-LocalUser "%v" -ea:SilentlyContinue | Remove-LocalUser -ea:SilentlyContinue; Exit 0
		`, user)

	return w.runCommand(winrm.Powershell(c))
}

// whoami gets identity of user running shell on remote endpoint, used to test connection.
func (w *WinRS) WhoAmI() error {
	return w.runCommand(winrm.Powershell(`[System.Security.Principal.WindowsIdentity]::GetCurrent().Name`))
}

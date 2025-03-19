package e2e

import (
	"bytes"
	"context"
	"crypto/md5" //nolint:gosec
	"encoding/base64"
	"encoding/hex"
	"errors"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/cenkalti/backoff/v4"
	"github.com/gofrs/uuid"
	"github.com/masterzen/winrm"
)

// winrs contains endpoint and credentials to create remote shell and execute commands.
type winrs struct {
	endpoint winrm.Endpoint
	username string
	password string
}

const HTTPSPort = 5986

func newWinRS(username, password, address string) winrs {
	return winrs{
		endpoint: winrm.Endpoint{
			Host:     address,
			Port:     HTTPSPort,
			HTTPS:    true,
			Insecure: true,
		},
		username: username,
		password: password,
	}
}

const connTimeout = time.Minute * 10

// newWinRMShell returns winrm shell with given config, connected to endpoint and ready for command execution.
func (w *winrs) newWinRMShell() (*winrm.Shell, error) {
	p := winrm.DefaultParameters
	p.TransportDecorator = func() winrm.Transporter {
		return &winrm.ClientNTLM{}
	}

	cl, err := winrm.NewClientWithParameters(&w.endpoint, w.username, w.password, p)
	if err != nil {
		return nil, err
	}

	var shell *winrm.Shell

	op := func() (e error) {
		shell, e = cl.CreateShell()

		return
	}
	bo := backoff.NewExponentialBackOff()
	ctx, cancel := context.WithTimeout(context.Background(), connTimeout)
	defer cancel()

	err = backoff.Retry(op, backoff.WithContext(bo, ctx))
	if err != nil {
		return nil, err
	}

	return shell, nil
}

// runCommand is wrapper for execute, passes os.stdout & os.stderr for output.
func (w *winrs) runCommand(c string) (err error) {
	return w.execute(os.Stdout, os.Stderr, c)
}

// runCommandS is wrapper for execute, passes os.stderr for output and return stdout as a string.
func (w *winrs) runCommandS(c string) (stdout string, err error) {
	var bufOut bytes.Buffer

	err = w.execute(&bufOut, os.Stderr, c)
	if err != nil {
		return
	}

	stdout = bufOut.String()

	return
}

func (w *winrs) runCommandSilently(c string) (err error) {
	return w.execute(io.Discard, io.Discard, c)
}

var errExitCode = errors.New("non zero exit code")

// execute create remote cmd shell and run provided command,
// to use powershell, command should be wrapped in winrm.powershell(COMMAND).
func (w *winrs) execute(stdout io.Writer, stderr io.Writer, c string) error {
	shell, err := w.newWinRMShell()
	if err != nil {
		return err
	}

	defer func() {
		errClose := shell.Close()
		if err == nil {
			err = errClose
		}
	}()

	var cmd *winrm.Command

	cmd, err = shell.Execute(c)
	if err != nil {
		return err
	}

	var wg sync.WaitGroup

	copier := func(w io.Writer, r io.Reader) {
		defer wg.Done()

		_, _ = io.Copy(w, r)
	}

	wg.Add(2) //nolint:gomnd
	go copier(stdout, cmd.Stdout)
	go copier(stderr, cmd.Stderr)

	cmd.Wait()
	wg.Wait()

	if cmd.ExitCode() != 0 {
		return fmt.Errorf("%w: %v", errExitCode, cmd.ExitCode())
	}

	return nil
}

var errChckSum = errors.New("md5 checksum mismatch")

// compareFile compares remote and local file by calculating md5 checksum.
func (w *winrs) compareFile(remote, local string) error {
	file, err := openFile(local)
	if err != nil {
		return err
	}

	hash := md5.New() //nolint:gosec
	if _, err = io.Copy(hash, file); err != nil {
		return err
	}
	localMD5 := hex.EncodeToString(hash.Sum(nil))

	var remoteMD5 string
	remoteMD5, err = w.getFileMD5(remote)
	if err != nil {
		return err
	}

	if localMD5 != remoteMD5 {
		return errChckSum
	}

	return nil
}

func (w *winrs) getFileMD5(path string) (res string, err error) {
	c := fmt.Sprintf(`
		(Get-FileHash -Path "%s" -Algorithm:MD5 |Select-Object -ExpandProperty Hash).ToLower()`, path)
	var stdout string
	stdout, err = w.runCommandS(winrm.Powershell(c))
	if err != nil {
		return
	}

	res = strings.Trim(stdout, "\r\n")

	return
}

// winrmSetMaxEnvelope configures remote WinRM service to use maximum envelope size.
func (w *winrs) winrmSetMaxEnvelope() error {
	return w.runCommand(winrm.Powershell(`Set-Item WSMan:\localhost\MaxEnvelopeSizekb -Value 8192`))
}

// winrmSetMaxOps configures remote WinRM service to use maximum concurrent operations per user.
func (w *winrs) winrmSetMaxOps() error {
	c := `Set-Item WSMan:\localhost\Service\MaxConcurrentOperationsPerUser -Value 4294967295`

	return w.runCommand(winrm.Powershell(c))
}

// uploadFile copies file on remote windows instance via WinRM, by:
// * tweaking a bit remote WinRM service
// * reading local file
// * encoding chunks in base64 string
// * generating remote file line by line
// * restoring file
// * comparing md5 checksum.
func (w *winrs) uploadFile(dst, src string) error {
	file, err := openFile(src)
	if err != nil {
		return err
	}

	var name uuid.UUID
	name, err = uuid.NewV4()
	if err != nil {
		return err
	}
	cmdTempFilePath := fmt.Sprintf("%%temp%%\\%s", name.String())
	pwshTempFilePath := fmt.Sprintf("$ENV:TEMP\\%s", name.String())

	err = w.winrmSetMaxEnvelope()
	if err != nil {
		return err
	}

	err = w.winrmSetMaxOps()
	if err != nil {
		return err
	}

	err = w.generateRestoreFile(cmdTempFilePath, file)
	if err != nil {
		return err
	}

	err = w.processRestoreFile(dst, pwshTempFilePath)
	if err != nil {
		return err
	}

	err = w.compareFile(dst, src)
	if err != nil {
		return err
	}

	return nil
}

var errUnexpDir = errors.New("provided directory, not file")

// openFile is a helper, which return error when you try to open directory, not file.
func openFile(path string) (*os.File, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}

	var fileStat os.FileInfo
	fileStat, err = file.Stat()
	if err != nil {
		return nil, err
	}

	if fileStat.IsDir() {
		return nil, errUnexpDir
	}

	return file, nil
}

// generateRestoreFile creates file on remote endpoint by filling it with base64 encoded strings of original file.
func (w *winrs) generateRestoreFile(path string, f io.Reader) (err error) {
	// for command to fit envelope size we have in `winrm` package
	buffSize := ((8150 - len(path)) / 4) * 3 //nolint:gomnd
	buff := make([]byte, buffSize)

	for {
		var n int
		n, err = f.Read(buff)
		if errors.Is(err, io.EOF) {
			err = nil
		}
		if n == 0 {
			break
		}

		// we generate `cmd` commands to fill temporary file line by line
		b64str := base64.StdEncoding.EncodeToString(buff[:n])
		c := fmt.Sprintf(`echo %s >> "%s"`, b64str, path)
		if err = w.runCommand(c); err != nil {
			break
		}
	}

	return
}

// processRestoreFile restore by converting base64 encoded file into original one.
func (w *winrs) processRestoreFile(dst, src string) error {
	// awful implementation, but to make it better
	// we must rewrite winrm package
	c := fmt.Sprintf(`
		$ErrorActionPreference = 'Stop'

		$tmp = "%s"
		$dst = "%s"

		if (Test-Path -Path $dst -PathType:Container) {
			Exit 1
		}

		Remove-Item $dst -ErrorAction:SilentlyContinue
		New-Item -Type:File -Path $dst -Force | Out-Null

		try {
			$r = [System.IO.File]::OpenText($tmp)
			$w = [System.IO.File]::OpenWrite($dst)

			for() {
				$l = $r.ReadLine()
				if ( [string]::IsNullOrEmpty($l) ) { break }
				$b = [System.Convert]::FromBase64String($l)
				$w.write($b, 0, $b.Length)
			}
		} catch {
			Remove-Item $dst -ErrorAction:SilentlyContinue
			throw $Error[0]
		} finally {
			Remove-Item $tmp -ErrorAction:SilentlyContinue
			$r.Close()
			$w.Close()
		}
	`, src, dst)

	return w.runCommand(winrm.Powershell(c))
}

func (w *winrs) stopProcess(pid int) error {
	return w.runCommand(winrm.Powershell(fmt.Sprintf("Stop-Process -Id %s", strconv.Itoa(pid))))
}

func (w *winrs) checkProcess(pid int) error {
	return w.runCommand(winrm.Powershell(fmt.Sprintf("Get-Process -Id %s | Out-Null", strconv.Itoa(pid))))
}

func (w *winrs) getProcessID(name string) (pid int, err error) {
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

func (w *winrs) startProcess(path string, args ...string) (pid int, err error) {
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

func (w *winrs) removeRegKey(path string) error {
	c := fmt.Sprintf(`Get-Item -Path %s -ea:SilentlyContinue | Remove-Item -ea:SilentlyContinue; Exit 0`, path)

	return w.runCommand(winrm.Powershell(c))
}

func (w *winrs) addLocalGroupMember(group, user string) error {
	c := fmt.Sprintf(`Add-LocalGroupMember -Member "%s" -Name "%s"`, user, group)

	return w.runCommand(winrm.Powershell(c))
}

func (w *winrs) checkMemberOfAdministrators(user string) (ok bool, err error) {
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

func (w *winrs) createAdministrator(user, pass string) error {
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

func (w *winrs) removeUser(user string) error {
	c := fmt.Sprintf(`
		Get-LocalUser "%v" -ea:SilentlyContinue | Remove-LocalUser -ea:SilentlyContinue; Exit 0
		`, user)

	return w.runCommand(winrm.Powershell(c))
}

// whoami gets identity of user running shell on remote endpoint, used to test connection.
func (w *winrs) whoami() error {
	return w.runCommand(winrm.Powershell(`[System.Security.Principal.WindowsIdentity]::GetCurrent().Name`))
}

// sendCtrlC sends CtrlC signal to process with name `Agent`
func (w *winrs) startAndStopByCtrlC(path string) error {
	c := fmt.Sprintf(`
		$startAgent = New-ScheduledTaskAction -Execute "%s" -Argument "start --log-level debug"
		Register-ScheduledTask -TaskName "startAgent" -Action $startAgent -User "System" | Out-Null
		Start-ScheduledTask startAgent
		Start-Sleep -Seconds 5
		Stop-ScheduledTask startAgent
		Unregister-ScheduledTask startAgent -Confirm:$false`, path)

	return w.runCommand(winrm.Powershell(c))
}

func (w *winrs) startService(name string) error {
	return w.runCommand(winrm.Powershell(fmt.Sprintf(`Start-Service -Name %s`, name)))
}

func (w *winrs) getService(name string) error {
	return w.runCommandSilently(winrm.Powershell(fmt.Sprintf(`Get-Service -Name "%s"`, name)))
}

// checkService searches for given service name and throws error if its status not `Running`.
func (w *winrs) checkService(name string) error {
	c := fmt.Sprintf(`
		$ErrorActionPreference = 'Stop'

		$Service = Get-Service -Name %s
		$Status = $Service.Status
		$IsRunning = $Status -eq 'Running'

		if (!$IsRunning) {
			throw '{0} is {1}, expected "Running"' -f $Service, $Status
		}`, name)

	return w.runCommand(winrm.Powershell(c))
}

func (w *winrs) stopService(name string) error {
	return w.runCommand(winrm.Powershell(fmt.Sprintf(`Stop-Service -Name %s`, name)))
}

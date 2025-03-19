package winrs

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"io"
	"os"
	"sync"
	"time"

	"github.com/cenkalti/backoff/v4"
	"github.com/masterzen/winrm"
)

// WinRS contains endpoint and credentials to create remote shell and execute commands.
type WinRS struct {
	endpoint winrm.Endpoint
	username string
	password string
	timeout  time.Duration
}

const HTTPSPort = 5986

func NewWinRS(username, password, address string, timeout time.Duration) *WinRS {
	return &WinRS{
		endpoint: winrm.Endpoint{
			Host:     address,
			Port:     HTTPSPort,
			HTTPS:    true,
			Insecure: true,
		},
		username: username,
		password: password,
		timeout:  timeout,
	}
}

// connect returns winrm shell with given config, connected to endpoint and ready for command execution.
func (w *WinRS) connect() (*winrm.Shell, error) {
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
		if e != nil {
			_, _ = fmt.Fprintf(os.Stdout, "failed to connect: %v\n", e)
		}

		return
	}
	bo := backoff.NewExponentialBackOff()
	ctx, cancel := context.WithTimeout(context.Background(), w.timeout)
	defer cancel()

	err = backoff.Retry(op, backoff.WithContext(bo, ctx))
	if err != nil {
		return nil, err
	}

	return shell, nil
}

// runCommand is wrapper for execute, passes os.stdout & os.stderr for output.
func (w *WinRS) runCommand(c string) (err error) {
	return w.execute(os.Stdout, os.Stderr, c)
}

// runCommandS is wrapper for execute, passes os.stderr for output and return stdout as a string.
func (w *WinRS) runCommandS(c string) (stdout string, err error) {
	var bufOut bytes.Buffer

	err = w.execute(&bufOut, os.Stderr, c)
	if err != nil {
		return
	}

	stdout = bufOut.String()

	return
}

func (w *WinRS) runCommandSilently(c string) (err error) {
	return w.execute(io.Discard, io.Discard, c)
}

var errExitCode = errors.New("non zero exit code")

// execute create remote cmd shell and run provided command,
// to use powershell, command should be wrapped in winrm.powershell(COMMAND).
func (w *WinRS) execute(stdout io.Writer, stderr io.Writer, c string) error {
	shell, err := w.connect()
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

// Add echo function for connection testing
func (w *WinRS) Echo() error {
	return w.runCommand(winrm.Powershell(`Write-Host "Connected"`))
}

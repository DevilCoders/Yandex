package pssh

import (
	"bufio"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"os"
	"os/exec"
	"strings"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
)

type YubikeyAction = func(ctx context.Context) error

type PsshIface interface {
	// pssh run 'ls' C@cloud_prod_nbs-control_myt
	Run(ctx context.Context, cmd string, target string) ([]string, error)

	CopyFile(ctx context.Context, source string, target string) error
}

////////////////////////////////////////////////////////////////////////////////

type psshImpl struct {
	logutil.WithLog

	path   string
	params []string

	yubikeyAction YubikeyAction
}

func (p *psshImpl) readStdout(
	ctx context.Context,
	stdout io.Reader,
) ([]string, error) {
	var out []string

	reader := bufio.NewReader(stdout)

	for {
		line, err := reader.ReadString('\n')
		line = strings.TrimRight(line, "\n")

		if len(line) != 0 {
			out = append(out, line)
			p.LogDbg(ctx, "[pssh.readStdout] '%v'", line)
		}
		if err == io.EOF {
			break
		}
		if err != nil {
			p.LogError(ctx, "[pssh.readStdout] ReadString: %v", err)

			break
		}
	}

	return out, nil
}

func (p *psshImpl) readStderr(
	ctx context.Context,
	stderr io.Reader,
) error {
	reader := bufio.NewReader(stderr)

	var errs []string

	for {
		line, err := reader.ReadString('\n')
		line = strings.TrimSpace(line)

		if strings.Contains(line, "You are going to be authenticated via federation-id") {
			continue
		}

		if strings.HasPrefix(line, "Issuing new session certificate") {
			const prompt = "Please touch yubikey"

			buf := make([]byte, len(prompt))
			_, err = io.ReadFull(reader, buf)

			if err != nil {
				if err == io.EOF {
					break
				}
				return fmt.Errorf("pssh.readStderr. io.ReadFull: %w", err)
			}

			if p.yubikeyAction != nil {
				err = p.yubikeyAction(ctx)
				if err != nil {
					p.LogError(ctx, "[pssh.readStderr] yubikeyAction: %v", err)
				}
			}

			if p.yubikeyAction == nil || err != nil {
				fmt.Fprintln(os.Stderr, line)
				fmt.Fprintln(os.Stderr, prompt)
			}

			line, err = reader.ReadString('\n')
			if err != nil && err != io.EOF {
				return fmt.Errorf("pssh.readStderr. can't read OK: %w", err)
			}

			line = strings.TrimSpace(line)

			if line != "OK" {
				p.LogWarn(ctx, "[pssh.readStderr] %v", line)
			}

			continue
		}

		if len(line) != 0 && !strings.HasPrefix(line, "Completed ") {
			p.LogWarn(ctx, "WARN [pssh.readStderr] %v", line)

			if strings.Contains(line, "ERROR") ||
				strings.Contains(line, "Remote exited without signal") {

				errs = append(errs, line)
			}
		}

		if err == io.EOF {
			break
		}

		if err != nil {
			return fmt.Errorf("pssh.readStderr. ReadString: %w", err)
		}
	}

	if len(errs) != 0 {
		return errors.New(strings.Join(errs, "\n"))
	}

	return nil
}

func (p *psshImpl) runCmd(
	ctx context.Context,
	command *exec.Cmd,
) ([]string, error) {
	stderr, err := command.StderrPipe()
	if err != nil {
		return nil, fmt.Errorf("StderrPipe: %w", err)
	}

	stdout, err := command.StdoutPipe()
	if err != nil {
		return nil, fmt.Errorf("StdoutPipe: %w", err)
	}

	err = command.Start()
	if err != nil {
		return nil, fmt.Errorf("can't start command: %w", err)
	}

	done := make(chan error)

	go func() {
		done <- p.readStderr(ctx, stderr)
		close(done)
	}()

	out, err := p.readStdout(ctx, stdout)
	if err != nil {
		return nil, err
	}

	err = <-done
	if err != nil {
		return nil, fmt.Errorf("command finished with error: %w", err)
	}

	err = command.Wait()
	if err != nil {
		return nil, fmt.Errorf("pssh finished with error: %w", err)
	}

	return out, err
}

func (p *psshImpl) Run(
	ctx context.Context,
	cmd string,
	target string,
) ([]string, error) {

	p.LogDbg(ctx, "[Run] %v on %v", cmd, target)

	allparams := append(p.params, "run", "--format", "json", cmd, target)

	command := exec.CommandContext(
		ctx,
		p.path,
		allparams...,
	)

	stdout, err := command.StdoutPipe()
	if err != nil {
		return nil, fmt.Errorf("can't get stdout: %w", err)
	}

	stderr, err := command.StderrPipe()
	if err != nil {
		return nil, fmt.Errorf("can't get stderr: %w", err)
	}

	err = command.Start()
	if err != nil {
		return nil, fmt.Errorf("can't start pssh: %w", err)
	}

	done := make(chan error)

	go func() {
		done <- p.readStderr(ctx, stderr)
		close(done)
	}()

	r := &struct {
		Host       string `json:"host"`
		Stdout     string `json:"stdout"`
		Stderr     string `json:"stderr"`
		Error      string `json:"error"`
		ExitStatus int32  `json:"exit_status"`
	}{}

	if err := json.NewDecoder(stdout).Decode(r); err != nil {
		return nil, fmt.Errorf("can't unmarshal response: %w", err)
	}

	p.LogDbg(ctx, "[Run] response: '%v'", r)

	err = <-done
	if err != nil {
		return nil, fmt.Errorf("command finished with error: %w", err)
	}

	err = command.Wait()
	if err != nil {
		return nil, fmt.Errorf("pssh finished with error: %w", err)
	}

	r.Stdout = strings.TrimSpace(r.Stdout)
	r.Stderr = strings.TrimSpace(r.Stderr)

	if len(r.Stderr) != 0 {
		p.LogError(ctx, "[Run] stderr: %v", r.Stderr)
	}

	if r.ExitStatus != 0 {
		return nil, fmt.Errorf(
			"command finished with error: %v %v",
			r.ExitStatus,
			r.Error,
		)
	}

	p.LogDbg(ctx, "[Run] stdout: '%v'", r.Stdout)

	if len(r.Stdout) == 0 {
		return nil, nil
	}

	return strings.Split(r.Stdout, "\n"), nil
}

func (p *psshImpl) CopyFile(
	ctx context.Context,
	source string,
	target string,
) error {
	p.LogDbg(ctx, "[CopyFile] '%v' to %v", source, target)

	allparams := append(p.params, "scp", source, target)
	command := exec.CommandContext(ctx, p.path, allparams...)
	lines, err := p.runCmd(ctx, command)
	if err != nil {
		return fmt.Errorf("pssh.CopyFile. %w", err)
	}
	if len(lines) != 1 || strings.TrimSpace(lines[0]) != "Completed 1/1" {
		return fmt.Errorf("pssh.CopyFile. Unexpected output: %v", lines)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func New(log nbs.Log, path string, yubikeyAction YubikeyAction) PsshIface {
	return &psshImpl{
		WithLog: logutil.WithLog{
			Log: log,
		},
		path:          path,
		params:        nil,
		yubikeyAction: yubikeyAction,
	}
}

func NewWithParams(log nbs.Log, path string, params []string, yubikeyAction YubikeyAction) PsshIface {
	return &psshImpl{
		WithLog: logutil.WithLog{
			Log: log,
		},
		path:          path,
		params:        params,
		yubikeyAction: yubikeyAction,
	}
}

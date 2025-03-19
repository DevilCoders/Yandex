package call

import (
	"bytes"
	"context"
	"fmt"
	"os"
	"os/exec"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/salt"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/library/go/core/log"
)

type Config struct {
	StopTimeout encodingutil.Duration `json:"stop_timeout" yaml:"stop_timeout"`
}

func DefaultConfig() Config {
	return Config{StopTimeout: encodingutil.FromDuration(time.Minute)}
}

type Manager struct {
	changes chan salt.Change
	lock    sync.Mutex
	pids    map[int]struct{}
	l       log.Logger
	cfg     Config
}

var _ agent.CallManager = &Manager{}

func New(l log.Logger, cfg Config) *Manager {
	return &Manager{
		l:       l,
		changes: make(chan salt.Change, 100),
		pids:    make(map[int]struct{}),
		cfg:     cfg,
	}
}

func (m *Manager) Changes() <-chan salt.Change {
	return m.changes
}

func (m *Manager) Run(ctx context.Context, job salt.Job, progress bool) {
	// CommandContext will kill the process if the context becomes done before the command complete,
	// but it will only Kill the main PID, not its children.
	// There is solution:
	// https://medium.com/@felixge/killing-a-child-process-and-all-of-its-children-in-go-54079af94773
	cmd := exec.CommandContext(ctx, job.Name, job.Args...)
	setProcessGroupID(cmd)

	stdout := &bytes.Buffer{}
	stderr := &bytes.Buffer{}
	cmd.Stdout = stdout
	cmd.Stderr = stderr
	startedAt := time.Now()
	if startErr := cmd.Start(); startErr != nil {
		m.changes <- salt.Change{
			Job:       job,
			StartedAt: startedAt,
			Result: salt.Result{
				// Process state and exit_code
				// available after a call to Wait or Run.
				ExitCode:   -1,
				Error:      startErr,
				Stdout:     stdout.String(),
				Stderr:     stderr.String(),
				FinishedAt: time.Now(),
			},
		}
		m.l.Warnf("'%s' fails to start: %s", cmd, startErr)
		return
	}
	m.l.Infof("'%s' started. its pid is %d", cmd, cmd.Process.Pid)
	if progress {
		m.changes <- salt.Change{
			Job:       job,
			PID:       cmd.Process.Pid,
			StartedAt: startedAt,
			Progress:  fmt.Sprintf("'%s' started. its pid is %d", cmd, cmd.Process.Pid),
		}
	}

	m.lock.Lock()
	m.pids[cmd.Process.Pid] = struct{}{}
	m.lock.Unlock()

	go func() {
		waitErr := cmd.Wait()

		m.lock.Lock()
		delete(m.pids, cmd.Process.Pid)
		m.lock.Unlock()

		processState := cmd.ProcessState.String()
		if waitErr != nil {
			m.l.Warnf("'%s' exited with %s (%s): %s", cmd, waitErr, processState, stderr.String())
		} else {
			m.l.Infof("'%s' exited successfully (%s)", cmd, processState)
		}

		m.changes <- salt.Change{
			Job:       job,
			PID:       cmd.Process.Pid,
			StartedAt: startedAt,
			Result: salt.Result{
				ExitCode:   cmd.ProcessState.ExitCode(),
				Error:      waitErr,
				Stdout:     stdout.String(),
				Stderr:     stderr.String(),
				FinishedAt: time.Now(),
			},
		}

	}()
}

func (m *Manager) Shutdown() {
	m.lock.Lock()
	pids := make([]int, 0, len(m.pids))
	for pid := range m.pids {
		pids = append(pids, pid)
	}
	m.lock.Unlock()

	m.l.Infof("There %d commands to shutdown", len(pids))
	for _, pid := range pids {
		if pid <= 1 {
			m.l.Warnf("Suspicious pid %d. Ignore it", pid)
			continue
		}
		p, err := os.FindProcess(pid)
		if err != nil {
			m.l.Warnf("Unable to find %d process: %s", pid, err)
			continue
		}
		m.l.Infof("Sending SIGINT to a %d process", p.Pid)
		if err := p.Signal(os.Interrupt); err != nil {
			m.l.Warnf("SIGINT sending to %d fails: %s", p.Pid, err)
		}

		if err := p.Release(); err != nil {
			m.l.Warnf("Release on process fails with %s", err)
		}

		time.Sleep(m.cfg.StopTimeout.Duration)

		p, err = os.FindProcess(pid)
		if err != nil {
			m.l.Debugf("%d reacts to SIGINT. That process not found: %s", pid, err)
			continue
		}
		m.l.Infof("process %d still running trying to kill it", pid)
		err = terminateProcess(p)
		if err != nil {
			m.l.Warnf("%d command kill is unsuccessful: %s", pid, err)
		} else {
			m.l.Infof("%d command killed successfully", pid)
		}
	}
}

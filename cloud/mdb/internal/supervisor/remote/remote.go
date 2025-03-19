package remote

import (
	"bytes"
	"os/exec"

	"a.yandex-team.ru/cloud/mdb/internal/supervisor"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type remote struct {
	l        log.Logger
	host     string
	user     string
	password string
}

var _ supervisor.Services = &remote{}

// New constructs supervisor control
func New(host, user, password string, l log.Logger) supervisor.Services {
	return &remote{
		host:     host,
		user:     user,
		password: password,
		l:        l,
	}
}

func (r *remote) Start(name string) error {
	cmd := exec.Command("supervisorctl", "-s", r.host, "-u", r.user, "-p", r.password, "start", name)
	if err := r.execute(cmd); err != nil {
		return xerrors.Errorf("failed to start process %s: %w", name, err)
	}

	return nil
}

func (r *remote) Stop(name string) error {
	cmd := exec.Command("supervisorctl", "-s", r.host, "-u", r.user, "-p", r.password, "stop", name)
	if err := r.execute(cmd); err != nil {
		return xerrors.Errorf("failed to stop process %s: %w", name, err)
	}

	return nil
}

func (r *remote) execute(cmd *exec.Cmd) error {
	r.l.Debugf("Executing command: %s %s", cmd.Path, cmd.Args)

	var out bytes.Buffer
	cmd.Stdout = &out

	if err := cmd.Run(); err != nil {
		return err
	}

	r.l.Debugf("Command output: %s", out.String())
	return nil
}

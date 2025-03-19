package runner

import (
	"fmt"
	"io/ioutil"
	"os/exec"
	"strings"

	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/porto"
	l "a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ porto.Runner = &PortoctlRunner{}

// PortoctlRunner allow direct run command on porto container
type PortoctlRunner struct {
	log l.Logger
}

// New create new portoctl implementation
func New(log l.Logger) *PortoctlRunner {
	return &PortoctlRunner{
		log: log,
	}
}

// RunCommandOnPortoContainer execute shell command in container
func (pr *PortoctlRunner) RunCommandOnPortoContainer(container, name string, args ...string) error {
	pr.log.Infof("execute on container %s, command %s, args %v", container, name, args)
	cmd := exec.Command("/usr/sbin/portoctl", "shell", container)
	w, err := cmd.StdinPipe()
	if err != nil {
		return err
	}
	execCmd := fmt.Sprintf("%s %s", name, strings.Join(args, " "))
	if _, err = w.Write([]byte(execCmd)); err != nil {
		return err
	}
	if err = w.Close(); err != nil {
		return err
	}
	err = cmd.Run()
	if err != nil {
		return err
	}
	return nil
}

// RunCommandOnDom0 execute shell command in dom0
func (pr *PortoctlRunner) RunCommandOnDom0(name string, args ...string) error {
	msg := fmt.Sprintf("run command '%s %s'", name, strings.Join(args, " "))
	cmd := exec.Command(name, args...)
	stdout, err := cmd.StdoutPipe()
	if err != nil {
		return xerrors.Errorf("failed get out pipe for %s: %s", msg, err)
	}
	stderr, err := cmd.StderrPipe()
	if err != nil {
		return xerrors.Errorf("failed get err pipe for %s: %s", msg, err)
	}
	err = cmd.Start()
	if err != nil {
		return xerrors.Errorf("start failed for %s: %v", msg, err)
	}

	stdoutData, err := ioutil.ReadAll(stdout)
	if err != nil {
		pr.log.Warnf("failed read out pipe for %s: %s", msg, err)
	}
	stderrData, err := ioutil.ReadAll(stderr)
	if err != nil {
		pr.log.Warnf("failed read err pipe for %s: %s", msg, err)
	}

	if len(stdoutData) > 0 {
		pr.log.Infof("output pipe for %s:\n%s", msg, string(stdoutData))
	}
	if len(stderrData) > 0 {
		pr.log.Infof("error pipe for %s:\n%s", msg, string(stderrData))
	}

	err = cmd.Wait()
	if err != nil {
		return xerrors.Errorf("failed %s: %v", msg, err)
	}
	return nil
}

package skcli

import (
	"bytes"
	"io/ioutil"
	"os/exec"
	"path"

	"a.yandex-team.ru/cloud/mdb/deploy/saltkeys/internal/saltkeys"
	"a.yandex-team.ru/library/go/core/log"
)

// Cli implements interface for managing minion keys on local salt-master
type Cli struct {
	l log.Logger
}

func New(l log.Logger) *Cli {
	return &Cli{l: l}
}

func (c *Cli) execute(cmd *exec.Cmd) error {
	c.l.Debugf("Executing command: %s %s", cmd.Path, cmd.Args)

	var out bytes.Buffer
	cmd.Stdout = &out

	if err := cmd.Run(); err != nil {
		return err
	}

	c.l.Debugf("Command output: %s", out.String())
	return nil
}

// Accept minion key
func (c *Cli) Accept(fqdn string) error {
	// #nosec G204
	cmd := exec.Command("salt-key", "-y", "--include-rejected", "-a", fqdn)
	if err := c.execute(cmd); err != nil {
		c.l.Errorf("Failed to accept key for minion %s: %s", fqdn, err)
		return err
	}

	c.l.Infof("Accepted key for minion %s", fqdn)
	return nil
}

// Reject minion key
func (c *Cli) Reject(fqdn string) error {
	// #nosec G204
	cmd := exec.Command("salt-key", "-y", "--include-accepted", "-r", fqdn)
	if err := c.execute(cmd); err != nil {
		c.l.Errorf("Failed to reject key for minion %s: %s", fqdn, err)
		return err
	}

	c.l.Infof("Rejected key for minion %s", fqdn)
	return nil
}

// Delete minion key
func (c *Cli) Delete(fqdn string) error {
	// #nosec G204
	cmd := exec.Command("salt-key", "-y", "-d", fqdn)
	if err := c.execute(cmd); err != nil {
		c.l.Errorf("Failed to delete key for minion %s: %s", fqdn, err)
		return err
	}

	c.l.Infof("Deleted key for minion %s", fqdn)
	return nil
}

// List minions for specified path
func (c *Cli) List(s saltkeys.StateID) ([]string, error) {
	fis, err := ioutil.ReadDir(s.KeyPath())
	if err != nil {
		return nil, err
	}

	files := make([]string, 0, len(fis))
	for _, fi := range fis {
		if fi.IsDir() {
			continue
		}

		files = append(files, fi.Name())
	}

	return files, nil
}

// Key returns public key of specific minion in specific state
func (c *Cli) Key(fqdn string, s saltkeys.StateID) (string, error) {
	d, err := ioutil.ReadFile(path.Join(s.KeyPath(), fqdn))
	return string(d), err
}

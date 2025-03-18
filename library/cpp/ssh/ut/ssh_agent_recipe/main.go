package main

import (
	"context"
	"encoding/gob"
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"os/exec"
	"os/signal"
	"syscall"
	"time"

	"github.com/cenkalti/backoff/v4"
	"github.com/gofrs/uuid"
	"github.com/spf13/pflag"
	"golang.org/x/crypto/ssh"
	"golang.org/x/crypto/ssh/agent"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/test/recipe"
	"a.yandex-team.ru/library/go/test/yatest"
)

const (
	infoFilename = "ssh_agent_info.gob"
)

var (
	keys    []string
	isChild bool
)

var _ recipe.Recipe = (*sshAgentRecipe)(nil)

type sshAgentRecipe struct{}

type sshAgentInfo struct {
	Addr string
	PID  int
}

func (r sshAgentRecipe) Start() error {
	if isChild {
		zapConfig := zap.ConsoleConfig(log.DebugLevel)
		zapConfig.OutputPaths = []string{
			yatest.OutputPath("ssh-agent.log"),
		}
		l, err := zap.New(zapConfig)
		if err != nil {
			return fmt.Errorf("can't create logger: %w", err)
		}

		ctx, cancel := context.WithCancel(context.Background())
		defer cancel()

		waitServer, addr, err := newServer(ctx, l)
		if err != nil {
			l.Error("can't start server", log.Error(err))
			return err
		}

		err = writeAgentInfo(&sshAgentInfo{
			Addr: addr,
			PID:  os.Getpid(),
		})
		if err != nil {
			l.Error("can't save ssh agent info", log.Error(err))
			return err
		}

		l.Info("server started")
		stopChan := make(chan os.Signal, 1)
		signal.Notify(stopChan, syscall.SIGINT)

		<-stopChan
		cancel()
		<-waitServer
		return nil
	}

	l, err := zap.New(zap.ConsoleConfig(log.DebugLevel))
	if err != nil {
		return fmt.Errorf("can't create logger: %w", err)
	}

	bi, err := startChild(l)
	if err != nil {
		return err
	}

	recipe.SetEnv("SSH_AUTH_SOCK", bi.Addr)
	recipe.SetEnv("SSH_AGENT_PID", fmt.Sprint(bi.PID))
	return nil
}

func (r sshAgentRecipe) Stop() error {
	if _, err := os.Stat(infoFilename); err != nil {
		if os.IsNotExist(err) {
			// that's fine
			return nil
		}
		return err
	}

	bi, err := readAgentInfo()
	if err != nil {
		return err
	}

	return kill(bi.PID)
}

func kill(pid int) error {
	process, err := os.FindProcess(pid)
	if err != nil {
		return err
	}
	return process.Signal(syscall.SIGINT)
}

func startChild(l log.Logger) (*sshAgentInfo, error) {
	_ = os.Remove(infoFilename)
	errs := make(chan error, 1)
	out := make(chan *sshAgentInfo, 1)
	go func() {
		path, err := os.Executable()
		if err != nil {
			errs <- fmt.Errorf("failed to get currect executable: %w", err)
			return
		}

		cmd := exec.Command(path, append(os.Args[1:], "--ssh-agent-recipe-is-a-child")...)
		if err = cmd.Start(); err != nil {
			errs <- err
			return
		}

		waiter := func() error {
			pi, err := readAgentInfo()
			if err != nil {
				return err
			}

			out <- pi
			return nil
		}

		notify := func(err error, _ time.Duration) {
			l.Warn("wait ssh srv srv fail", log.Error(err))
		}

		b := backoff.WithMaxRetries(backoff.NewConstantBackOff(2*time.Second), 10)
		err = backoff.RetryNotify(waiter, b, notify)
		if err != nil {
			errs <- err
		}
	}()

	select {
	case err := <-errs:
		return nil, fmt.Errorf("failed to start child: %w", err)
	case pi := <-out:
		return pi, nil
	}
}

func newServer(ctx context.Context, l log.Logger) (<-chan struct{}, string, error) {
	keyring := agent.NewKeyring()
	for _, key := range keys {
		rawKey, err := os.ReadFile(yatest.SourcePath(key))
		if err != nil {
			return nil, "", fmt.Errorf("read %q key: %w", key, err)
		}

		priv, err := ssh.ParseRawPrivateKey(rawKey)
		if err != nil {
			return nil, "", fmt.Errorf("parse %q key: %w", key, err)
		}

		err = keyring.Add(agent.AddedKey{
			PrivateKey: priv,
		})
		if err != nil {
			return nil, "", fmt.Errorf("add %q key: %w", key, err)
		}
	}

	rnd, err := uuid.NewV4()
	if err != nil {
		return nil, "", err
	}

	socketPath := fmt.Sprintf("%s.sock", rnd)
	ln, err := net.Listen("unix", socketPath)
	if err != nil {
		return nil, socketPath, fmt.Errorf("listen: %w", err)
	}

	conns := make(chan net.Conn)
	go func() {
		for {
			c, err := ln.Accept()
			switch {
			case err == nil:
				conns <- c
			case errors.Is(err, net.ErrClosed), errors.Is(err, io.ErrClosedPipe):
				return
			default:
				l.Error("could not accept connection to agent", log.Error(err))
			}
		}
	}()

	waitCh := make(chan struct{})
	go func() {
		defer close(waitCh)

		for {
			select {
			case <-ctx.Done():
				_ = ln.Close()
				_ = os.RemoveAll(socketPath)
				return
			case c := <-conns:
				go func() { _ = agent.ServeAgent(keyring, c) }()
			}
		}
	}()

	return waitCh, socketPath, nil
}

func readAgentInfo() (*sshAgentInfo, error) {
	f, err := os.Open(infoFilename)
	if err != nil {
		return nil, err
	}
	defer func() { _ = f.Close() }()

	var bi sshAgentInfo
	err = gob.NewDecoder(f).Decode(&bi)
	if err != nil {
		return nil, err
	}
	return &bi, nil
}

func writeAgentInfo(bi *sshAgentInfo) error {
	f, err := os.Create(infoFilename)
	if err != nil {
		return err
	}
	defer func() { _ = f.Close() }()

	return gob.NewEncoder(f).Encode(bi)
}

func main() {
	pflag.CommandLine.ParseErrorsWhitelist.UnknownFlags = true
	pflag.BoolVar(&isChild, "ssh-agent-recipe-is-a-child", false, "")
	pflag.StringSliceVar(&keys, "keys", nil, "preloaded SSH keys")
	pflag.Parse()
	recipe.Run(sshAgentRecipe{})
}

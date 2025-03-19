package main

import (
	"os"
	"os/signal"
	"syscall"

	l "a.yandex-team.ru/library/go/core/log"
)

const (
	sigChannelSize = 4
)

func handleSignals(logger l.Logger) {
	sigChannel := make(chan os.Signal, sigChannelSize)
	signal.Notify(sigChannel,
		syscall.SIGHUP,
		syscall.SIGINT,
		syscall.SIGTERM,
		syscall.SIGQUIT,
	)
	for {
		sig := <-sigChannel
		logger.Infof("Got signal: %s", sig)
		// TODO: handle signals properly
		if sig != syscall.SIGHUP {
			break
		}
	}
}

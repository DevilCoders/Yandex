//go:build !darwin && !freebsd && !linux
// +build !darwin,!freebsd,!linux

package logrotate

import (
	"fmt"
	"net/url"
	"os"

	"go.uber.org/zap"
)

func RegisterLogrotateSink(sig ...os.Signal) error {
	return fmt.Errorf("logrotate sink is not supported on your platform")
}

func RegisterNamedLogrotateSink(schemeName string, sig ...os.Signal) error {
	return fmt.Errorf("logrotate sink is not supported on your platform")
}

func NewLogrotateSink(u *url.URL, sig ...os.Signal) (zap.Sink, error) {
	return nil, fmt.Errorf("logrotate sink is not supported on your platform")
}

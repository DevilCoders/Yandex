package net

import (
	"net"
	"strconv"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func IncrementPort(hostport string, change int16) (string, error) {
	host, portStr, err := net.SplitHostPort(hostport)
	if err != nil {
		return "", xerrors.Errorf("split host port %q: %w", hostport, err)
	}
	port, err := strconv.ParseInt(portStr, 10, 16)
	if err != nil {
		return "", xerrors.Errorf("parse address port %q: %w", portStr, err)
	}

	port += int64(change)
	if port < 1 {
		return "", xerrors.Errorf("port underflow: %d", port)
	}
	if port > 65535 {
		return "", xerrors.Errorf("port overflow: %d", port)
	}

	return net.JoinHostPort(host, strconv.FormatInt(port, 10)), nil
}

package internetwork

import (
	"context"
	"errors"
	"net"
	"sync/atomic"
	"time"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

var (
	// ErrListenerClosed is returned to clients and servers is the listener closed
	ErrListenerClosed = errors.New("inmemory listener closed")
)

// InternalNetwork is internal inmemory listener
type InternalNetwork interface {
	net.Listener
	DialgRPC(name string, d time.Duration) (net.Conn, error)
	DialContext(ctx context.Context, name, addr string) (net.Conn, error)
	Dial(name, addr string) (net.Conn, error)
}

type interNetwork struct {
	queue   chan net.Conn
	onClose chan struct{}

	closed uint64
}

// New returns inmemory Listener
func New(backlog int) InternalNetwork {
	return &interNetwork{
		queue:   make(chan net.Conn, backlog),
		onClose: make(chan struct{}),
		closed:  uint64(0),
	}
}

func (i *interNetwork) Accept() (net.Conn, error) {
	select {
	case <-i.onClose:
		return nil, ErrListenerClosed
	case c := <-i.queue:
		return c, nil
	}
}

func (i *interNetwork) Addr() net.Addr {
	return &net.UnixAddr{Name: "", Net: "unix"}
}

func (i *interNetwork) Close() error {
	if atomic.CompareAndSwapUint64(&i.closed, 0, 1) {
		close(i.onClose)
	}
	return nil
}

// DialgRPC has signature to be passed as grcp.WithDialer option
func (i *interNetwork) DialgRPC(name string, d time.Duration) (net.Conn, error) {
	ctx, cancel := context.WithTimeout(context.Background(), d)
	defer cancel()
	return i.DialContext(ctx, "", name)
}

// Dial implements Dialer interface
func (i *interNetwork) Dial(network, addr string) (net.Conn, error) {
	return i.DialContext(context.Background(), network, addr)
}

// DialContext implements Dialer inerface
func (i *interNetwork) DialContext(ctx context.Context, _, _ string) (net.Conn, error) {
	serverSide, clientSide := net.Pipe()
	select {
	case i.queue <- serverSide:
		return clientSide, nil
	case <-i.onClose:
		if err := serverSide.Close(); err != nil {
			log.G(ctx).Error("serverSide.Close failed", zap.Error(err))
		}
		if err := clientSide.Close(); err != nil {
			log.G(ctx).Error("clientSide.Close failed", zap.Error(err))
		}
		return nil, ErrListenerClosed
	}
}

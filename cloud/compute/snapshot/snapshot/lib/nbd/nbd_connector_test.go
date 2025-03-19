package nbd

import (
	"context"
	"errors"
	"io"
	"io/ioutil"
	"net"
	"os"
	"path/filepath"
	"testing"
	"time"

	"golang.org/x/xerrors"

	"encoding/binary"

	"github.com/stretchr/testify/assert"
	"go.uber.org/zap"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

func TestOkConnection(t *testing.T) {
	start := initTest(t)
	defer start.cancel()

	at := assert.New(t)
	ctx := start.ctx

	start.onAccept = func(conn net.Conn) {
		_ = writeHandshake(conn)
	}

	client, err := NewNbdConnector(ctx, func(ctx context.Context) (context.Context, error) { return ctx, nil }, start.sock)
	at.NotNil(client)
	at.NoError(err)
	_ = client.Close(ctx)
}

func TestDelayConnection(t *testing.T) {
	start := initTest(t)
	defer start.cancel()

	at := assert.New(t)
	ctx := start.ctx

	start.onAccept = func(conn net.Conn) {
		_ = writeHandshake(conn)
	}
	start.StopListen()

	timeCtx, timeCancel := context.WithTimeout(ctx, time.Second*5)
	defer timeCancel()

	go func() {
		time.Sleep(time.Second * 1)
		start.StartListen(t)
	}()

	client, err := NewNbdConnector(timeCtx, func(ctx context.Context) (context.Context, error) { return ctx, nil }, start.sock)
	at.NotNil(client)
	at.NoError(err)
	_ = client.Close(ctx)
}

func TestReconnect(t *testing.T) {
	start := initTest(t)
	defer start.cancel()

	at := assert.New(t)
	ctx := start.ctx

	firstConnectedCtx, firstConnectedCtxCancel := context.WithCancel(context.Background())
	start.onAccept = func(conn net.Conn) {
		t.Log("write handshake")
		_ = writeHandshake(conn)
	}

	timeCtx, timeCancel := context.WithTimeout(ctx, time.Second*5)
	defer timeCancel()

	start.StopListen()

	client, err := NewNbdConnector(timeCtx, func(ctx context.Context) (context.Context, error) {
		if ctx.Err() != nil {
			return nil, xerrors.Errorf("Try start with cancelled context: %w", ctx.Err())
		}
		t.Log("Start listen in start func")
		start.StartListen(t)

		// on first connection
		if firstConnectedCtx.Err() == nil {
			stopListenerCtx, stopListenerCtxCancel := context.WithCancel(context.Background())
			go func() {
				// wait connection
				<-firstConnectedCtx.Done()
				t.Log("Stop listen after first connected closed")
				start.StopListen()
				stopListenerCtxCancel()
			}()
			return stopListenerCtx, nil
		}

		return ctx, nil
	}, start.sock)
	at.NotNil(client)
	at.NotNil(client.client)
	at.NoError(err)

	oldClient := client.client

	t.Log("Close first connection")
	// close connection after success start
	firstConnectedCtxCancel()

	t.Log("Wait for reconnect")
	time.Sleep(time.Second * 2)

	t.Log("Check if created new client (new connection)")
	at.NotEqual(oldClient, client.client)

	t.Log("Close client from test code")
	err = client.Close(ctx)
	at.NoError(err)
}

type initStruct struct {
	ctx               context.Context
	cancel            context.CancelFunc
	dir               string
	onAccept          func(conn net.Conn)
	sock              string
	listenerCtxCancel context.CancelFunc
	listener          net.Listener
}

func (s *initStruct) StartListen(t *testing.T) {
	if s.listener != nil {
		t.Fatal("listen already started")
	}

	listener, err := net.Listen("unix", s.sock)
	if err != nil {
		t.Fatal(err)
	}

	listenerCtx, listenerCtxCancel := context.WithCancel(s.ctx)
	s.listenerCtxCancel = listenerCtxCancel
	s.listener = listener

	go func() {
		for {
			conn, err := listener.Accept()
			if listenerCtx.Err() != nil {
				return
			}
			if err != nil {
				panic(err)
			}
			if s.onAccept != nil {
				go s.onAccept(conn)
			}
		}
	}()
}

func (s *initStruct) StopListen() {
	if s.listener != nil {
		s.listenerCtxCancel()
		s.listenerCtxCancel = nil

		err := s.listener.Close()
		if err != nil {
			panic(err)
		}
		s.listener = nil
	}
}

func initTest(t *testing.T) *initStruct {
	logger := zaptest.NewLogger(t, zaptest.WrapOptions(zap.Development()))
	ctx, cancel := context.WithCancel(context.Background())
	ctx = ctxlog.WithLogger(ctx, logger)
	dir, err := ioutil.TempDir("", "")
	if err != nil {
		panic(err)
	}

	sock := filepath.Join(dir, "sock")

	res := initStruct{
		ctx:  ctx,
		dir:  dir,
		sock: sock,
	}

	cancelFunc := func() {
		cancel()
		res.StopListen()
		err := os.RemoveAll(dir)
		if err != nil {
			t.Error(err)
		}
	}
	res.cancel = cancelFunc

	res.StartListen(t)

	return &res
}

func writeHandshake(w io.WriteCloser) error {
	var err error
	const oldStyleNegotiationMagic uint64 = 0x00420281861253
	const diskSize uint64 = 100
	const reservedZeroBytesSize = 124

	write := func(data interface{}) {
		err = binary.Write(w, binary.BigEndian, data)
	}

	write([]byte("NBDMAGIC"))
	write(oldStyleNegotiationMagic)
	write(diskSize)
	write(uint32(0))                           // flags
	write(make([]byte, reservedZeroBytesSize)) // reserved
	return err
}

func TestWrapExecToStartQemuNbdFunc(t *testing.T) {
	at := assert.New(t)

	start := initTest(t)
	defer start.cancel()
	waitFile := "test"

	testError := errors.New("test")

	// start ok
	f := wrapExecToStartQemuNbdFunc(time.Second, start.dir, waitFile, func(ctx context.Context) error {
		_ = ioutil.WriteFile(filepath.Join(start.dir, waitFile), []byte("123"), 0600)
		time.Sleep(time.Second * 2)
		return testError
	})
	retCtx, err := f(start.ctx)
	at.NotNil(retCtx)
	at.NoError(err)

	// start with cancelled context
	f = wrapExecToStartQemuNbdFunc(time.Second, start.dir, "test", func(ctx context.Context) error { return nil })
	ctxCancelled, ctxCancelledCancel := context.WithCancel(start.ctx)
	ctxCancelledCancel()
	retCtx, err = f(ctxCancelled)
	at.Nil(retCtx)
	at.Error(err)

	// start timeout
	f = wrapExecToStartQemuNbdFunc(time.Millisecond, start.dir, waitFile, func(ctx context.Context) error {
		time.Sleep(time.Millisecond * 10)
		return nil
	})
	retCtx, err = f(start.ctx)
	at.Nil(retCtx)
	at.Error(err)

	// start failed
	f = wrapExecToStartQemuNbdFunc(time.Second, start.dir, waitFile, func(ctx context.Context) error {
		return testError
	})
	retCtx, err = f(start.ctx)
	at.Nil(retCtx)
	at.True(xerrors.Is(err, testError))
}

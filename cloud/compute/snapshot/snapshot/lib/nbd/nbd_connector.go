package nbd

import (
	"net"
	"os"
	"path/filepath"
	"sync"
	"time"

	"github.com/fsnotify/fsnotify"

	"a.yandex-team.ru/library/go/core/xerrors"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"

	"golang.org/x/net/context"

	"a.yandex-team.ru/cloud/compute/snapshot/internal/nbdclient"
)

const (
	nbdStartTimeout      = time.Minute
	nbdConnectTimeout    = time.Second
	nbdClientStopTimeout = time.Second
)

type empty struct{}

// StartQemuNbdFunc Start qemu-nbd process in background
// Qemu-nbd must stop when ctx cancelled
// return
// context - if start succesful, will close when process stop
// error - if start was failed
type StartQemuNbdFunc func(ctx context.Context) (context.Context, error)

// NbdConnector is keep-alived connect to image with qemu-nbd
// it reconnect if need and restart qemu-nbd if errors
type NbdConnector struct {
	qemuNbdStart      StartQemuNbdFunc
	qemuNbdSocketPath string
	logger            *zap.Logger

	stopCtx          context.Context
	stopCtxCancel    func()
	nbdServerStopCtx context.Context

	starts chan empty
	size   int64

	mu     sync.Mutex
	client *nbdclient.Client
	conn   net.Conn
}

func (c *NbdConnector) Size() int64 {
	return c.size
}

func (c *NbdConnector) nbdStart() error {
	nbdProcessCtx, err := c.qemuNbdStart(log.WithLogger(c.stopCtx, c.logger))
	if err != nil {
		return err
	}
	go c.nbdKeepAlive(nbdProcessCtx)
	return nil
}

func (c *NbdConnector) nbdStop(ctx context.Context) {
	if c.stopCtxCancel != nil {
		log.G(ctx).Debug("Canceling stopCtx due to NbdConnector.nbdStop")
		c.stopCtxCancel()
	}
}

func (c *NbdConnector) nbdKeepAlive(nbdProcessCtx context.Context) {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()

	nbdCtx := log.WithLogger(c.stopCtx, c.logger)

	for {
		c.starts <- empty{}
		<-nbdProcessCtx.Done()
		c.logger.Debug("Qemu-nbd stoped")

		// guard from restarts flood
		select {
		case <-c.stopCtx.Done():
			c.logger.Debug("Stop qemu-nbd keepalive")
			return
		case <-ticker.C:
			// pass for restart cmd
		}

		var err error
		nbdProcessCtx, err = c.qemuNbdStart(nbdCtx)
		log.DebugError(c.logger, err, "Restart qemu-nbd")
		if err != nil {
			err = c.Close(c.stopCtx)
			log.DebugError(c.logger, err, "Self close nbd connector")
		}
	}
}

func (c *NbdConnector) clientKeepAlive(firstClientReady chan empty) {
	firstClientSended := false
	retryAfter := time.Second
	retry := time.NewTimer(retryAfter)
	retry.Stop()

	for {
		select {
		case <-c.stopCtx.Done():
			// pass
		case <-c.starts:
			c.logger.Debug("retry connect to nbd because restart container")
			// pass
		case <-retry.C:
			c.logger.Debug("retry connect to nbd because timer after last error")
			// pass
		}
		if c.stopCtx.Err() != nil {
			_ = closeClient(c.logger, c.conn, c.client)
			c.logger.Debug("return from clientKeepAlive by context end")
			return
		}

		c.mu.Lock()
		go func(conn net.Conn, client *nbdclient.Client) {
			_ = closeClient(c.logger, conn, client)
		}(c.conn, c.client)

		netConn, err := net.DialTimeout("unix", c.qemuNbdSocketPath, nbdConnectTimeout)
		stat, _ := os.Stat(c.qemuNbdSocketPath)
		var statMode = "<nil>"
		if stat != nil {
			statMode = stat.Mode().String()
		}
		log.DebugError(c.logger, err, "Connect to nbd process", zap.String("filepath", c.qemuNbdSocketPath),
			zap.Any("statMode", statMode))
		var negotiation string
		if err == nil {
			c.conn = netConn
			//setDeadlines(c.logger, c.conn, time.Now())
			c.client, err = nbdclient.NewClient(c.conn)
			if err == nil && !firstClientSended {
				c.size = c.client.Size()
				firstClientReady <- empty{}
				firstClientSended = true
				negotiation = c.client.HandshakeNegotiation()
			}
		}

		c.mu.Unlock()
		log.DebugError(c.logger, err, "Create new nbd client", zap.String("negotiation", negotiation),
			zap.String("socket_path", c.qemuNbdSocketPath))
		if err != nil {
			retry.Reset(retryAfter)
		}
	}
}

func NewNbdConnector(ctx context.Context, startQemuNbd StartQemuNbdFunc, qemuNbdSocketPath string) (*NbdConnector, error) {
	logger := log.G(ctx).Named("nbd_connector")
	stopCtx, stopCtxCancel := context.WithCancel(context.Background())
	nbdServerStopCtx, nbdServerStopCtxCancel := context.WithCancel(context.Background())
	nbdServerStopCtx = log.WithLogger(nbdServerStopCtx, logger)

	connector := NbdConnector{
		qemuNbdStart:      startQemuNbd,
		qemuNbdSocketPath: qemuNbdSocketPath,
		client:            nil,
		starts:            make(chan empty),
		logger:            logger,
		stopCtx:           stopCtx,
		stopCtxCancel:     stopCtxCancel,
		nbdServerStopCtx:  nbdServerStopCtx,
	}

	err := connector.nbdStart()
	if err != nil {
		logger.Error("Can't start nbd connector", zap.Error(err))
		return nil, err
	}

	go func() {
		<-stopCtx.Done()
		// time for graceful shutdown
		time.Sleep(nbdClientStopTimeout)
		log.G(ctx).Debug("Canceling nbdServerStopCtx due to end of stopCtx")
		nbdServerStopCtxCancel()
	}()

	firstClientReady := make(chan empty)
	go connector.clientKeepAlive(firstClientReady)
	select {
	case <-ctx.Done():
		go func() {
			err := connector.Close(ctx)
			log.DebugError(logger, err, "Background close nbd connector after failed creation")
		}()
		return nil, xerrors.New("Can't create nbd client")
	case <-firstClientReady:
		return &connector, nil
	}
}

func (c *NbdConnector) ReadAt(p []byte, off int64) (n int, err error) {
	if c.stopCtx.Err() != nil {
		return 0, xerrors.Errorf("read from closed connector: %w", c.stopCtx.Err())
	}

	c.mu.Lock()
	client := c.client
	c.mu.Unlock()
	if client == nil {
		return 0, xerrors.New("Nil nbd client")
	}
	return client.ReadAt(p, off)
}

func (c *NbdConnector) Close(ctx context.Context) error {
	// close client
	err := closeClient(log.G(ctx), c.conn, c.client)
	log.DebugErrorCtx(ctx, err, "Close client")

	// close server
	log.G(ctx).Debug("Canceling stopCtx due to NbdConnector.Close")
	c.stopCtxCancel()
	<-c.nbdServerStopCtx.Done()
	return nil
}

func closeClient(logger *zap.Logger, conn net.Conn, client *nbdclient.Client) error {
	var err error
	if client != nil {
		err = client.Close()
		log.DebugError(logger, err, "Close nbd client")
	}
	if conn != nil {
		connCloseErr := conn.Close()
		log.DebugError(logger, connCloseErr, "Close connection for client")
		if err == nil {
			err = connCloseErr
		}
	}
	return err
}

func wrapExecToStartQemuNbdFunc(waitFileTimeout time.Duration, monDir, waitFile string, exec func(ctx context.Context) error) StartQemuNbdFunc {
	return func(ctx context.Context) (c context.Context, err error) {
		watcher, err := fsnotify.NewWatcher()
		log.DebugErrorCtx(ctx, err, "create watcher")
		if err != nil {
			return nil, err
		}
		defer watcher.Close()

		err = watcher.Add(monDir)
		log.DebugErrorCtx(ctx, err, "Add dir to watcher", zap.String("dir", monDir))
		if err != nil {
			return nil, err
		}

		timeout := time.NewTimer(waitFileTimeout)
		defer timeout.Stop()

		execErrChan := make(chan error, 1)
		execCtx, execCtxCancel := context.WithCancel(context.Background())
		go func() {
			startError := exec(ctx)
			execCtxCancel()
			execErrChan <- startError
		}()

		for {
			select {
			case <-ctx.Done():
				return nil, xerrors.Errorf("start cancelled: %w", err)
			case <-timeout.C:
				return nil, xerrors.New("start timeout")
			case err := <-execErrChan:
				return nil, xerrors.Errorf("start failed: %w", err)
			case event := <-watcher.Events:
				if filepath.Base(event.Name) == waitFile && event.Op == fsnotify.Create {
					return execCtx, nil
				}
			}
		}
	}
}

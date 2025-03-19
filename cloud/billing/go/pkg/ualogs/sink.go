package ualogs

import (
	"context"
	"errors"
	"fmt"
	"io"
	"net/url"
	"sync"
	"time"

	"github.com/jonboulle/clockwork"
	"go.uber.org/zap"
	"google.golang.org/grpc"

	uapb "a.yandex-team.ru/logbroker/unified_agent/plugins/grpc_input/proto"
)

// NewUnifiedAgentSink creates new sing for direct write to UnifiedAgent grpc endpoint
// URL should be like `logbroker://localhost:<port>?secret=<shared secret key>`
func NewUnifiedAgentSink(u *url.URL) (zap.Sink, error) {
	if u.User != nil {
		return nil, fmt.Errorf("user and password not allowed with unified agent URLs: got %v", u)
	}
	if u.Fragment != "" {
		return nil, fmt.Errorf("fragments not allowed with unified agent URLs: got %v", u)
	}

	query := u.Query()

	client := &uaSink{
		endpoint:  u.Host,
		secretKey: query.Get("secret"),
		clock:     clockwork.NewRealClock(),
	}

	if err := client.open(); err != nil {
		return nil, err
	}
	return client, nil
}

var _ zap.Sink = &uaSink{}

type uaSink struct {
	endpoint  string
	secretKey string

	mu sync.Mutex

	conn *grpc.ClientConn
	uc   uapb.UnifiedAgentServiceClient
	sess uapb.UnifiedAgentService_SessionClient

	sessionID      string
	seqNo          uint64
	committedSeqNo uint64

	clock clockwork.Clock

	syncCtx      context.Context
	syncCallback context.CancelFunc
}

func (c *uaSink) Write(p []byte) (int, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	err := c.write(p)
	if err == io.EOF { // EOF means streaming closed, reopen and try to send again
		err = c.write(p)
	}
	if err != nil {
		return 0, fmt.Errorf("failed to send data message: %w", err)
	}
	return len(p), nil
}

func (c *uaSink) Sync() error {
	// Synced part
	c.mu.Lock()
	// check if already in sync state
	if c.seqNo == c.committedSeqNo {
		c.mu.Unlock()
		return nil
	}
	// if no active sync wait start it by creating sync context
	if c.syncCtx == nil {
		c.syncCtx, c.syncCallback = context.WithTimeout(context.Background(), time.Second*5)
	}
	// remember context to prevent race
	ctx := c.syncCtx
	c.mu.Unlock()

	// wait for sync state
	<-ctx.Done()
	if ctx.Err() != context.Canceled {
		return ctx.Err()
	}
	return nil
}

func (c *uaSink) Close() error {
	c.mu.Lock()
	unlock := c.mu.Unlock
	defer func() {
		if unlock != nil {
			unlock()
		}
	}()

	if c.uc == nil { // already closed
		return nil
	}
	conn := c.conn
	c.uc = nil
	c.conn = nil
	defer func() {
		_ = conn.Close()
		c.sess = nil
	}()

	if c.sess != nil {
		// try to close session
		// do not use dropSession because it will fail sync call
		if err := c.sess.CloseSend(); err != nil {
			return fmt.Errorf("failed to close sink: %w", err)
		}
	}

	// Sync will lock mutex. So unlock it to prevent deadlock.
	unlock()
	unlock = nil
	return c.Sync()
}

func (c *uaSink) open() (err error) {
	c.mu.Lock()
	defer c.mu.Unlock()
	if c.uc != nil {
		return errors.New("already connected")
	}

	// Use insecure connection because of we go to sidecar
	opts := []grpc.DialOption{
		grpc.WithInsecure(),
	}

	c.conn, err = grpc.Dial(c.endpoint, opts...)
	if err != nil {
		return fmt.Errorf("failed to init grpc connection: %w", err)
	}

	c.uc = uapb.NewUnifiedAgentServiceClient(c.conn)
	defer func() {
		if err != nil {
			c.sess = nil
			_ = c.conn.Close()
		}
	}()
	return c.init()
}

func (c *uaSink) write(p []byte) error {
	if c.sess == nil {
		if err := c.init(); err != nil {
			return err
		}
	}

	c.seqNo++
	request := &uapb.Request{
		Request: &uapb.Request_DataBatch_{
			DataBatch: &uapb.Request_DataBatch{
				SeqNo:     []uint64{c.seqNo},
				Payload:   [][]byte{p},
				Timestamp: []uint64{uint64(c.clock.Now().UnixNano() / 1000)},
			},
		},
	}
	if err := c.sess.Send(request); err != nil {
		c.dropSession()
		return err
	}
	return nil
}

func (c *uaSink) init() error {
	if c.uc == nil {
		return errors.New("sink closed")
	}

	sess, err := c.uc.Session(context.Background(), []grpc.CallOption{}...)
	if err != nil {
		return fmt.Errorf("failed to init session: %w", err)
	}

	request := &uapb.Request{
		Request: &uapb.Request_Initialize_{
			Initialize: &uapb.Request_Initialize{
				SessionId:       c.sessionID,
				SharedSecretKey: c.secretKey,
			},
		},
	}

	err = sess.Send(request)
	if err != nil {
		return fmt.Errorf("failed to send init request: %w", err)
	}

	resp, err := sess.Recv()
	if err != nil {
		return fmt.Errorf("failed to get init response: %w", err)
	}

	// Here we initialized.
	initResp := resp.GetInitialized()
	c.sess = sess
	// This should be sended in case of reconnect
	c.sessionID = initResp.GetSessionId()
	// In initial state all sequences are committed and we in synced state
	c.seqNo = initResp.GetLastSeqNo()
	c.committedSeqNo = c.seqNo

	// Run ack response read cycle
	go c.readAcks(c.sess)

	return nil
}

func (c *uaSink) readAcks(sess uapb.UnifiedAgentService_SessionClient) {
	// Here we process streaming responses from agent
	for {
		resp, err := sess.Recv()
		if err != nil { // in case of any error session should be reinitialized
			c.syncDropSession(sess)
			return
		}
		ack := resp.GetAck()
		if ack != nil { // not ack responses are not interested
			c.syncCommitSeqNo(sess, ack.SeqNo)
		}
	}
}

// syncCommitSeqNo set last committed message number with sync and session instance check
func (c *uaSink) syncCommitSeqNo(sess uapb.UnifiedAgentService_SessionClient, seqNo uint64) {
	c.mu.Lock()
	defer c.mu.Unlock()
	if c.sess != sess {
		return
	}
	c.committedSeqNo = seqNo
	if c.committedSeqNo == c.seqNo {
		c.synced()
	}
}

// syncDropSession drop sink active session with sync and session instance check
func (c *uaSink) syncDropSession(sess uapb.UnifiedAgentService_SessionClient) {
	c.mu.Lock()
	defer c.mu.Unlock()
	if c.sess != sess {
		return
	}
	c.dropSession()
}

// dropSession removes session without sync
func (c *uaSink) dropSession() {
	_ = c.sess.CloseSend()
	c.sess = nil
	c.seqNo = 0
	c.committedSeqNo = 0
	c.synced()
}

// synced call callback if needed when all sended messages acked
func (c *uaSink) synced() {
	if c.syncCallback != nil {
		c.syncCallback()
	}
	c.syncCallback = nil
	c.syncCtx = nil
}

package statshandler

import (
	"fmt"
	"math/rand"
	"sync"
	"sync/atomic"
	"time"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"golang.org/x/net/context"
	"google.golang.org/grpc/stats"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

type connTag struct{}
type lazyLoggerTag struct{}

var recordsPool = sync.Pool{
	New: func() interface{} {
		return make([]lazyRecord, 0)
	},
}

type lazyRecord struct {
	lvl    zapcore.Level
	msg    string
	fields []zapcore.Field
}

type lazyLogger struct {
	records []lazyRecord

	mu sync.Mutex
}

func newLazyLogger() *lazyLogger {
	// if it is not ok, we can use nil buffer
	records := recordsPool.Get().([]lazyRecord)
	return &lazyLogger{
		records: records,
	}
}

func (l *lazyLogger) push(lvl zapcore.Level, msg string, fields ...zapcore.Field) {
	l.mu.Lock()
	l.records = append(l.records, lazyRecord{lvl: lvl, msg: msg, fields: fields})
	l.mu.Unlock()
}

func (l *lazyLogger) flush(log *zap.Logger) {
	tag := fmt.Sprintf("%0.16x", rand.Uint64())
	log = log.With(zap.String("tag", tag))
	l.mu.Lock()
	for _, lr := range l.records {
		log.Check(lr.lvl, lr.msg).Write(lr.fields...)
	}
	l.mu.Unlock()
}

func (l *lazyLogger) free() {
	l.mu.Lock()
	lr := l.records[:0]
	recordsPool.Put(lr)
	l.records = nil
	l.mu.Unlock()
}

type statsHandler struct {
	bytesSent     int64
	bytesReceived int64
}

// New returns a stats.Handler
func New() stats.Handler {
	// TODO: export metrics
	return &statsHandler{}
}

// TagRPC can attach some information to the given context.
// The context used for the rest lifetime of the RPC will be derived from
// the returned context.
func (h statsHandler) TagRPC(ctx context.Context, info *stats.RPCTagInfo) context.Context {
	return context.WithValue(ctx, lazyLoggerTag{}, newLazyLogger())
}

// HandleRPC processes the RPC stats.
func (h statsHandler) HandleRPC(ctx context.Context, s stats.RPCStats) {
	lazyLog, ok := ctx.Value(lazyLoggerTag{}).(*lazyLogger)
	if !ok {
		log.G(ctx).Debug("rpc begins", zap.Any("stats", s))
		return
	}

	// TODO: use const messages to reduce memory footprint
	switch s := s.(type) {
	case *stats.Begin:
		lazyLog.push(zap.DebugLevel, "rpc begins", zap.Time("time", s.BeginTime))
	case *stats.End:
		if s.Error != nil {
			lazyLog.push(zap.ErrorLevel, "rpc ends", zap.Error(s.Error), zap.Time("time", s.EndTime))
			lazyLog.flush(log.G(ctx))
		}
		lazyLog.free()
	// ingress traffic
	case *stats.InHeader:
		atomic.AddInt64(&h.bytesReceived, int64(s.WireLength))
		lazyLog.push(zap.DebugLevel, "in header", zap.Int("bytes", s.WireLength), zap.Time("time", time.Now()))
	case *stats.InPayload:
		atomic.AddInt64(&h.bytesReceived, int64(s.WireLength))
		lazyLog.push(zap.DebugLevel, "in payload", zap.Int("bytes", s.WireLength), zap.Time("time", time.Now()))
	case *stats.InTrailer:
		atomic.AddInt64(&h.bytesReceived, int64(s.WireLength))
		lazyLog.push(zap.DebugLevel, "in trailer", zap.Int("bytes", s.WireLength), zap.Time("time", time.Now()))
	// outgress traffic
	case *stats.OutHeader:
		// TODO: return WireLength if API changes
		// atomic.AddInt64(&h.bytesSent, int64(s.WireLength))
		lazyLog.push(zap.DebugLevel, "out header", zap.Int("bytes", 0), zap.Time("time", time.Now()))
	case *stats.OutPayload:
		atomic.AddInt64(&h.bytesSent, int64(s.WireLength))
		lazyLog.push(zap.DebugLevel, "out payload", zap.Int("bytes", s.WireLength), zap.Time("time", time.Now()))
	case *stats.OutTrailer:
		atomic.AddInt64(&h.bytesSent, int64(s.WireLength))
		lazyLog.push(zap.DebugLevel, "out trailer", zap.Int("bytes", s.WireLength), zap.Time("time", time.Now()))
	}
}

// TagConn can attach some information to the given context.
// The returned context will be used for stats handling.
// For conn stats handling, the context used in HandleConn for this
// connection will be derived from the context returned.
// For RPC stats handling,
//  - On server side, the context used in HandleRPC for all RPCs on this
// connection will be derived from the context returned.
//  - On client side, the context is not derived from the context returned.
func (h statsHandler) TagConn(ctx context.Context, info *stats.ConnTagInfo) context.Context {
	log.G(ctx).Debug("tag connection", zap.Stringer("local", info.LocalAddr), zap.Stringer("remote", info.RemoteAddr))
	tag := fmt.Sprintf("%0.16x", rand.Uint64())
	return context.WithValue(ctx, connTag{}, tag)
}

// HandleConn processes the Conn stats.
func (h statsHandler) HandleConn(ctx context.Context, connStats stats.ConnStats) {
	if tag, ok := ctx.Value(connTag{}).(string); ok {
		switch connStats.(type) {
		case *stats.ConnBegin:
			log.G(ctx).Debug("connection is established", zap.String("tag", tag))
		case *stats.ConnEnd:
			log.G(ctx).Debug("connection ends", zap.String("tag", tag))
		}
	}
}

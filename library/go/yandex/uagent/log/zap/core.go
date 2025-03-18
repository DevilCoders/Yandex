package uazap

import (
	"context"

	"go.uber.org/zap/zapcore"

	uaclient "a.yandex-team.ru/library/go/yandex/uagent/log/zap/client"
)

// Core for sending logs to unified agent.
// Don't forget to call Stop not to lose logs on shutdown.
type Core struct {
	zapcore.LevelEnabler
	enc zapcore.Encoder
	bg  *background
}

var _ zapcore.Core = (*Core)(nil)

func NewCore(enc zapcore.Encoder, client uaclient.Client, level zapcore.LevelEnabler, opts ...Option) *Core {
	var opt options
	opt.setDefaults()

	for _, o := range opts {
		o(&opt)
	}

	bg := newBackground(opt, client)

	return &Core{LevelEnabler: level, enc: enc, bg: bg}
}

func (c *Core) With(fields []zapcore.Field) zapcore.Core {
	clone := c.clone()
	for i := range fields {
		fields[i].AddTo(clone.enc)
	}
	return clone
}

func (c *Core) Check(entry zapcore.Entry, checkedEntry *zapcore.CheckedEntry) *zapcore.CheckedEntry {
	if c.Enabled(entry.Level) {
		return checkedEntry.AddCore(entry, c)
	}
	return checkedEntry
}

func (c *Core) Write(entry zapcore.Entry, fields []zapcore.Field) error {
	buf, err := c.enc.EncodeEntry(entry, fields)
	if err != nil {
		return err
	}
	payload := buf.Bytes()
	if len(payload) > 0 && payload[len(payload)-1] == '\n' {
		payload = payload[:len(payload)-1]
	}

	meta := c.bg.options.metaExtractor(entry, fields)
	message := uaclient.Message{
		Meta:    meta,
		Payload: payload,
		Time:    &entry.Time,
	}
	return c.bg.emplace(message)
}

func (c *Core) Sync() error {
	return c.bg.sync()
}

func (c *Core) Stat() *Stat {
	c.bg.stat.mu.Lock()
	defer c.bg.stat.mu.Unlock()

	return &Stat{
		ReceivedMessages: c.bg.stat.ReceivedMessages,
		ReceivedBytes:    c.bg.stat.ReceivedBytes,
		InflightMessages: c.bg.stat.InflightMessages - c.bg.clientStat.AckedMessages(),
		InflightBytes:    c.bg.stat.InflightBytes - c.bg.clientStat.AckedBytes(),
		DroppedMessages:  c.bg.stat.DroppedMessages + c.bg.clientStat.DroppedMessages(),
		DroppedBytes:     c.bg.stat.DroppedBytes + c.bg.clientStat.DroppedBytes(),
		ErrorsCount:      c.bg.stat.DroppedMessages + c.bg.clientStat.ErrorCount(),
	}
}

func (c *Core) Stop(_ context.Context) {
	_ = c.Sync()
	c.bg.stop()
}

func (c *Core) clone() *Core {
	return &Core{
		LevelEnabler: c.LevelEnabler,
		enc:          c.enc.Clone(),
		bg:           c.bg,
	}
}

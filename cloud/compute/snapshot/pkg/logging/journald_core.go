package logging

import (
	"github.com/coreos/go-systemd/journal"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var priorityMap = map[zapcore.Level]journal.Priority{
	zapcore.FatalLevel:  journal.PriEmerg,
	zapcore.PanicLevel:  journal.PriCrit,
	zapcore.DPanicLevel: journal.PriCrit,
	zapcore.ErrorLevel:  journal.PriErr,
	zapcore.WarnLevel:   journal.PriWarning,
	zapcore.InfoLevel:   journal.PriInfo,
	zapcore.DebugLevel:  journal.PriDebug,
}

// JournaldCore is an SSKV-like journald-based Core.
type JournaldCore struct {
	zapcore.LevelEnabler
	enc *Encoder
}

// NewJournaldCore returns a new JournaldCore instance.
func NewJournaldCore(enc *Encoder, enab zapcore.LevelEnabler) zapcore.Core {
	return &JournaldCore{
		LevelEnabler: enab,
		enc:          enc,
	}
}

// With returns a copy with additional fields.
func (c *JournaldCore) With(fields []zapcore.Field) zapcore.Core {
	clone := c.clone()
	clone.enc.Add(fields)
	return clone
}

// Check determines whether an entry should be logged.
func (c *JournaldCore) Check(ent zapcore.Entry, ce *zapcore.CheckedEntry) *zapcore.CheckedEntry {
	if c.Enabled(ent.Level) {
		return ce.AddCore(ent, c)
	}
	return ce
}

// Write sends a message through journald socket.
func (c *JournaldCore) Write(ent zapcore.Entry, fields []zap.Field) error {
	buf, err := c.enc.EncodeEntry(ent, fields)
	if err != nil {
		return err
	}

	return journal.Send(buf.String(), priorityMap[ent.Level], c.enc.GetMetaFields(ent, fields))
}

// Sync does nothing.
func (c *JournaldCore) Sync() error {
	return nil
}

func (c *JournaldCore) clone() *JournaldCore {
	return &JournaldCore{
		LevelEnabler: c.LevelEnabler,
		enc:          c.enc.Clone(),
	}
}

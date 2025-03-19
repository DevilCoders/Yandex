package logging

import (
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

// FileCore is an SSKV file-based core.
type FileCore struct {
	zapcore.LevelEnabler
	out zapcore.WriteSyncer
	enc *Encoder
}

// NewFileCore returns a new FileCore instance.
func NewFileCore(enc *Encoder, ws zapcore.WriteSyncer, enab zapcore.LevelEnabler) zapcore.Core {
	return &FileCore{
		LevelEnabler: enab,
		out:          ws,
		enc:          enc,
	}
}

// With returns a copy with additional fields.
func (c *FileCore) With(fields []zapcore.Field) zapcore.Core {
	clone := c.clone()
	clone.enc.Add(fields)
	return clone
}

// Check determines whether an entry should be logged.
func (c *FileCore) Check(ent zapcore.Entry, ce *zapcore.CheckedEntry) *zapcore.CheckedEntry {
	if c.Enabled(ent.Level) {
		return ce.AddCore(ent, c)
	}
	return ce
}

// Write writes an entry to underlying file.
func (c *FileCore) Write(ent zapcore.Entry, fields []zap.Field) error {
	buf, err := c.enc.EncodeEntry(ent, fields)
	if err != nil {
		return err
	}
	_, err = c.out.Write(buf.Bytes())
	buf.Free()
	if err != nil {
		return err
	}
	if ent.Level > zapcore.ErrorLevel {
		// Since we may be crashing the program, sync the output. Ignore Sync
		// errors, pending a clean solution to issue
		// https://github.com/uber-go/zap/issues/370
		_ = c.Sync()
	}
	return nil
}

// Sync flushes the file.
func (c *FileCore) Sync() error {
	return c.out.Sync()
}

func (c *FileCore) clone() *FileCore {
	return &FileCore{
		LevelEnabler: c.LevelEnabler,
		enc:          c.enc.Clone(),
		out:          c.out,
	}
}

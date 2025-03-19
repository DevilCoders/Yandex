package logging

import (
	"fmt"

	"a.yandex-team.ru/cloud/compute/go-common/logging/go-logging"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

// SnapshotColorMap is a preset of colors for terminal.
var SnapshotColorMap = map[zapcore.Level]Color{
	zapcore.FatalLevel:  logging.ColorRed,
	zapcore.PanicLevel:  logging.ColorRed,
	zapcore.DPanicLevel: logging.ColorRed,
	zapcore.ErrorLevel:  logging.ColorRed,
	zapcore.WarnLevel:   logging.ColorYellow,
	zapcore.InfoLevel:   logging.ColorGreen,
	zapcore.DebugLevel:  logging.ColorCyan,
}

// Color is a terminal color code.
type Color uint8

// ColorMap is a mapping of level to Color.
type ColorMap map[zapcore.Level]Color

// EncodeColor encodes a string with Color.
func EncodeColor(s string, color Color) string {
	return fmt.Sprintf("\x1b[%dm%s\x1b[0m", color, s)
}

// ColoredFileCore is a zap.Core adding colors to FileCore.
type ColoredFileCore struct {
	zapcore.LevelEnabler
	out      zapcore.WriteSyncer
	enc      *Encoder
	colorMap ColorMap
}

// NewColoredFileCore returns a new ColoredFileCore instance.
func NewColoredFileCore(enc *Encoder, ws zapcore.WriteSyncer, enab zapcore.LevelEnabler, colorMap ColorMap) zapcore.Core {
	return &ColoredFileCore{
		LevelEnabler: enab,
		out:          ws,
		enc:          enc,
		colorMap:     colorMap,
	}
}

// With returns a copy with additional fields.
func (c *ColoredFileCore) With(fields []zapcore.Field) zapcore.Core {
	clone := c.clone()
	clone.enc.Add(fields)
	return clone
}

// Write writes an entry to underlying file.
func (c *ColoredFileCore) Write(ent zapcore.Entry, fields []zap.Field) error {
	buf, err := c.enc.EncodeEntry(ent, fields)
	if err != nil {
		return err
	}

	_, err = c.out.Write([]byte(EncodeColor(buf.String(), c.colorMap[ent.Level])))
	buf.Free()
	if err != nil {
		return err
	}
	if ent.Level > zapcore.ErrorLevel {
		// Since we may be crashing the program, sync the output. Ignore Sync
		// errors, pending a clean solution to issue #370.
		_ = c.Sync()
	}
	return nil
}

// Check determines whether an entry should be logged.
func (c *ColoredFileCore) Check(ent zapcore.Entry, ce *zapcore.CheckedEntry) *zapcore.CheckedEntry {
	if c.Enabled(ent.Level) {
		return ce.AddCore(ent, c)
	}
	return ce
}

// Sync flushes the file.
func (c *ColoredFileCore) Sync() error {
	return c.out.Sync()
}

func (c *ColoredFileCore) clone() *ColoredFileCore {
	return &ColoredFileCore{
		LevelEnabler: c.LevelEnabler,
		enc:          c.enc.Clone(),
		out:          c.out,
		colorMap:     c.colorMap,
	}
}

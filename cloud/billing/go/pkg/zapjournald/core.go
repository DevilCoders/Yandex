package zapjournald

import (
	"fmt"
	"strings"

	"github.com/coreos/go-systemd/journal"
	"go.uber.org/zap/zapcore"
)

type Core struct {
	zapcore.LevelEnabler
	fields []zapcore.Field

	send func(string, journal.Priority, map[string]string) error
}

func NewCore(le zapcore.LevelEnabler) *Core {
	return &Core{
		LevelEnabler: le,
		send:         journal.Send,
	}
}

func (core *Core) With(fields []zapcore.Field) zapcore.Core {
	clone := core.clone()
	clone.fields = append(clone.fields, fields...)
	return clone
}

func (core *Core) Check(entry zapcore.Entry, checked *zapcore.CheckedEntry) *zapcore.CheckedEntry {
	if core.Enabled(entry.Level) {
		return checked.AddCore(entry, core)
	}
	return checked
}

func (core *Core) Write(entry zapcore.Entry, fields []zapcore.Field) error {
	var jrnlLevel journal.Priority
	switch entry.Level {
	case zapcore.DebugLevel:
		jrnlLevel = journal.PriDebug
	case zapcore.InfoLevel:
		jrnlLevel = journal.PriInfo
	case zapcore.WarnLevel:
		jrnlLevel = journal.PriWarning
	case zapcore.ErrorLevel:
		jrnlLevel = journal.PriErr
	case zapcore.DPanicLevel:
		jrnlLevel = journal.PriCrit
	case zapcore.PanicLevel:
		jrnlLevel = journal.PriCrit
	case zapcore.FatalLevel:
		jrnlLevel = journal.PriCrit
	default:
		return fmt.Errorf("unknown log level: %v", entry.Level)
	}

	var vars map[string]string
	if len(core.fields) > 0 || len(fields) > 0 {
		vars = make(map[string]string, len(core.fields)+len(fields))
		for _, f := range core.fields {
			addVar(vars, f)
		}
		for _, f := range fields {
			addVar(vars, f)
		}
	}

	return core.send(entry.Message, jrnlLevel, vars)
}

func (core *Core) Sync() error {
	return nil
}

func (core *Core) clone() *Core {
	return &Core{
		LevelEnabler: core.LevelEnabler,
		fields:       append([]zapcore.Field(nil), core.fields...),
		send:         core.send,
	}
}

func addVar(to map[string]string, f zapcore.Field) {
	name := fixVarName(f.Key)
	if name == "" {
		return
	}
	enc := stringEncoder{}
	f.AddTo(&enc)

	if enc.notSupported {
		return
	}
	to[name] = enc.value
}

func fixVarName(name string) string {
	if name == "" {
		return ""
	} else if name[0] == '_' {
		return ""
	}

	name = strings.ToUpper(name)
	for _, c := range name {
		if !(('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '_') {
			return ""
		}
	}
	return name
}

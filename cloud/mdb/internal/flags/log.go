package flags

import (
	"fmt"
	"sync"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	defaultLevel     = log.DebugLevel
	flagNameLogLevel = "loglevel"
)

var registerLogLevelFlagOnce sync.Once

// RegisterLogLevelFlagGlobal for command line. Has exactly-once semantics.
func RegisterLogLevelFlagGlobal() {
	registerLogLevelFlagOnce.Do(
		func() {
			RegisterLogLevelFlag(pflag.CommandLine)
		},
	)
}

// RegisterLogLevelFlag registers log level flag. Example:
//	--loglevel debug
func RegisterLogLevelFlag(fs *pflag.FlagSet) *string {
	return fs.String(flagNameLogLevel, log.DebugString, fmt.Sprintf("Set log level. Available: %v", log.Levels()))
}

func FlagLogLevel() (log.Level, bool, error) {
	if !pflag.CommandLine.Changed(flagNameLogLevel) {
		return defaultLevel, false, nil
	}

	str, err := pflag.CommandLine.GetString(flagNameLogLevel)
	if err != nil {
		return defaultLevel, false, xerrors.Errorf("failed to retrieve log level from command line argument: %w", err)
	}

	level, err := log.ParseLevel(str)
	if err != nil {
		return defaultLevel, false, xerrors.Errorf("parse log level from command line argument: %w", err)
	}

	return level, true, nil
}

package flags

import (
	"sync"

	"github.com/spf13/pflag"
)

const (
	flagNameGenerateConfig = "genconfig"
	flagNameConfigPath     = "config-path"
	flagNameConfig         = "config"
)

var (
	configPathValue *string
	configPathFlag  *pflag.Flag

	genConfigValue *string
	genConfigFlag  *pflag.Flag

	configValue *string
	configFlag  *pflag.Flag
)

var (
	registerConfigOnce        sync.Once
	registerConfigPathOnce    sync.Once
	registerGenConfigFlagOnce sync.Once
)

// RegisterConfigFlagGlobal for command line. Has exactly-once semantics.
func RegisterConfigFlagGlobal() {
	registerConfigOnce.Do(
		func() {
			registerConfigFlag(pflag.CommandLine)
		},
	)
}

// RegisterConfigPathFlagGlobal for command line. Has exactly-once semantics.
func RegisterConfigPathFlagGlobal() {
	registerConfigPathOnce.Do(
		func() {
			registerConfigPathFlag(pflag.CommandLine)
		},
	)
}

// RegisterGenConfigFlagGlobal for command line. Has exactly-once semantics.
func RegisterGenConfigFlagGlobal() {
	registerGenConfigFlagOnce.Do(
		func() {
			registerGenConfigFlag(pflag.CommandLine)
		},
	)
}

func registerConfigPathFlag(set *pflag.FlagSet) {
	configPathValue = set.String(flagNameConfigPath, "", "Path where to look for configuration files")
	configPathFlag = set.Lookup(flagNameConfigPath)
}

func registerConfigFlag(set *pflag.FlagSet) {
	configValue = set.String(flagNameConfig, "", "Path to configuration file")
	configFlag = set.Lookup(flagNameConfig)
}

func registerGenConfigFlag(fs *pflag.FlagSet) {
	genConfigValue = fs.String(flagNameGenerateConfig, "", "Generate default config to specified directory")
	genConfigFlag = fs.Lookup(flagNameGenerateConfig)
	genConfigFlag.NoOptDefVal = "."
}

// ConfigFlag checks if --config flag is set and returns its value
func ConfigFlag() (bool, string) {
	if configFlag != nil && configFlag.Changed {
		return true, *configValue
	}
	return false, ""
}

// ConfigFlag checks if --config-path flag is set and returns its value
func ConfigPathFlag() (bool, string) {
	if configPathFlag != nil && configPathFlag.Changed {
		return true, *configPathValue
	}
	return false, ""
}

// ConfigFlag checks if --genconfig flag is set and returns its value
func GenConfigFlag() (bool, string) {
	if genConfigFlag != nil && !genConfigFlag.Changed {
		return false, ""
	}
	return true, *genConfigValue
}

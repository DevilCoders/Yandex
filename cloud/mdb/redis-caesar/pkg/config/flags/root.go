package flags

import "github.com/spf13/cobra"

// nolint: gochecknoglobals
var persistent *Persistent

// Persistent is a set of flags that common for all commands.
type Persistent struct {
	ConfigFile string
	LogLevel   string
}

// Root is a set of flags that contains PersistenFlags and specific for root command flags.
type Root struct {
	*Persistent
}

func ConfigureRoot(command *cobra.Command) *Root {
	persistent = &Persistent{}
	flags := &Root{
		Persistent: persistent,
	}
	command.PersistentFlags().StringVarP(&(persistent.ConfigFile), "config", "c", "", "config file")
	command.PersistentFlags().StringVarP(&(persistent.LogLevel), "loglevel", "l", "Warn", "logging level (Trace|Debug|Info|Warn|Error|Fatal)")

	return flags
}

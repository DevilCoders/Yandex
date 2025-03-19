package cli

import (
	"context"
	"fmt"
	"os"
	"strings"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/pretty"
	"a.yandex-team.ru/library/go/core/buildinfo"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	flagNameLogShowAll  = "logshowall"
	flagNameDryRun      = "dryrun"
	flagNameShortDryRun = "n" // nop
)

var (
	flagValueLogShowAll bool
	flagValueDryRun     bool
)

// Env is a cli command runtime environment
type Env struct {
	appOpts    []app.AppOption
	defCfgPath string // used for custom config

	*app.App
	Logger        log.Logger  // Deprecated: use app's L()
	Config        interface{} // Deprecated: use WithConfigLoad
	OutMarshaller pretty.Marshaller
	RootCmd       *Command
}

// RunFunc defines cli command function signature
type RunFunc func(ctx context.Context, env *Env, cmd *cobra.Command, args []string)

// Command describes one cli command
type Command struct {
	Cmd         *cobra.Command
	SubCommands []*Command
	Run         RunFunc
}

func NewWithOptions(ctx context.Context, rootCmd *Command, options ...Option) *Env {
	// Add version command
	rootCmd.Cmd.AddCommand(VersionCmd)
	rootCmd.Cmd.AddCommand(CompletionCmd)

	env := &Env{RootCmd: rootCmd}
	env.build(ctx)

	for _, o := range options {
		o(env)
	}

	cobra.OnInitialize(env.initCli)
	return env
}

// GetConfigPath return full path to config file
func (e *Env) GetConfigPath() string {
	_, path := config.PathByFlags(e.defCfgPath)
	return path
}

// IsDryRunMode return true if dry run mode and required only test run
func (e *Env) IsDryRunMode() bool {
	return flagValueDryRun
}

func (e *Env) build(ctx context.Context) {
	c := e.RootCmd
	if c.Cmd != nil && c.Run != nil {
		c.Cmd.Run = func(cmd *cobra.Command, args []string) {
			if e.App != nil {
				ctx = e.App.ShutdownContext()
			}
			c.Run(ctx, e, cmd, args)
		}
	}

	for _, subcmd := range c.SubCommands {
		subcmd.Build(ctx, e)
		c.Cmd.AddCommand(subcmd.Cmd)
	}
}

// Build wraps run function and recursively builds command tree structure
func (c *Command) Build(ctx context.Context, env *Env) {
	if c.Cmd != nil && c.Run != nil {
		c.Cmd.Run = func(cmd *cobra.Command, args []string) {
			c.Run(ctx, env, cmd, args)
		}
	}

	for _, subcmd := range c.SubCommands {
		subcmd.Build(ctx, env)
		c.Cmd.AddCommand(subcmd.Cmd)
	}
}

// initCli prepare base cli load
func (e *Env) initCli() {
	defaultConfig := app.DefaultConfig()
	defaultConfig.Logging.Level = log.InfoLevel

	options := []app.AppOption{app.WithConfig(&defaultConfig), app.WithNoFlagParse(), app.WithLoggerConstructor(defaultLoggerF)}
	options = append(options, e.appOpts...)
	a, err := app.New(options...)
	if err != nil {
		fmt.Printf("init failed: %v\n", err)
		os.Exit(1)
	}
	e.App = a
	e.Logger = a.L()
	go a.WaitForStop()
}

//default logger constructor, should be before user-provided AppOptions
var defaultLoggerF = func(cfg app.LoggingConfig) (log.Logger, error) {
	var constructor = app.DefaultCLILoggerConstructor()
	if flagValueLogShowAll {
		constructor = app.DefaultToolLoggerConstructor()
	}

	logger, err := constructor(cfg)
	return logger, err
}

// VersionCmd is a sub-command to print version information
var VersionCmd = &cobra.Command{
	Use:   "version",
	Short: "Display version information and exit",
	Run: func(*cobra.Command, []string) {
		fmt.Print(buildinfo.Info.ProgramVersion)
		fmt.Printf("Date: %s\n", buildinfo.Info.Date)
	},
}

// CompletionCmd is sub-command to generate shell Completion
var CompletionCmd = &cobra.Command{
	Use:   "completion [bash|zsh|fish|powershell]",
	Short: "Generate completion script",
	Long: strings.ReplaceAll(`To load completions:

Bash:

$ source <(yourprogram completion bash)

# To load completions for each session, execute once:
Linux:
  $ yourprogram completion bash > /etc/bash_completion.d/yourprogram
MacOS:
  $ yourprogram completion bash > /usr/local/etc/bash_completion.d/yourprogram

Zsh:

$ source <(yourprogram completion zsh)

# To load completions for each session, execute once:
$ yourprogram completion zsh > "${fpath[1]}/_yourprogram"

Fish:

$ yourprogram completion fish | source

# To load completions for each session, execute once:
$ yourprogram completion fish > ~/.config/fish/completions/yourprogram.fish

`, "yourprogram", os.Args[0]),
	DisableFlagsInUseLine: true,
	ValidArgs:             []string{"bash", "zsh", "fish", "powershell"},
	Args:                  cobra.ExactValidArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		switch args[0] {
		case "bash":
			_ = cmd.Root().GenBashCompletion(os.Stdout)
		case "zsh":
			_ = cmd.Root().GenZshCompletion(os.Stdout)
		case "fish":
			_ = cmd.Root().GenFishCompletion(os.Stdout, true)
		case "powershell":
			_ = cmd.Root().GenPowerShellCompletion(os.Stdout)
		}
	},
}

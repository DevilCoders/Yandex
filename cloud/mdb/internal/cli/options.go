package cli

import (
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
)

type Option = func(*Env)

func WithFlagLogShowAll() Option {
	return func(env *Env) {
		env.RootCmd.Cmd.PersistentFlags().BoolVar(
			&flagValueLogShowAll,
			flagNameLogShowAll,
			false,
			"show detailed log line with timestamp and level of message",
		)
	}
}

func WithConfigLoad(dst app.AppConfig, defCfgPath string) Option {
	return func(env *Env) {
		flags.RegisterConfigFlagGlobal()
		env.appOpts = append(env.appOpts, app.WithConfigLoad(defCfgPath), app.WithConfig(dst))
	}
}

// WithCustomConfig registers --config flag, but does not load config.
//
func WithCustomConfig(defCfgPath string) Option {
	return func(env *Env) {
		flags.RegisterConfigFlagGlobal()
		env.defCfgPath = defCfgPath
	}
}

// WithExtraAppOptions adds options to app. Use when you want to add sentry, metrics, etc.
func WithExtraAppOptions(opts ...app.AppOption) Option {
	return func(env *Env) {
		env.appOpts = append(env.appOpts, opts...)
	}
}

func WithFlagDryRun() Option {
	return func(env *Env) {
		env.RootCmd.Cmd.PersistentFlags().BoolVarP(
			&flagValueDryRun,
			flagNameDryRun,
			flagNameShortDryRun,
			false,
			"parse inputs, perform read-only operations but do not do any modifications",
		)
	}
}

// DefaultToolOptions returns cli options suitable for most tools
func DefaultToolOptions(cfg app.AppConfig, filename string) []Option {
	return []Option{
		WithConfigLoad(cfg, filename),
	}
}

package app

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/mdb-disklock/internal/disklock"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	cmdClose   = initClose()
	cmdMount   = initMount()
	cmdOpen    = initOpen()
	cmdUnmount = initUnmount()
	cmdStart   = initStart()
	cmdStop    = initStop()
	cmdFormat  = initFormat()
)

func initClose() *cli.Command {
	cmd := &cobra.Command{
		Use:   "close",
		Short: "Closes disks. Disks must be unmounted.",
	}

	return &cli.Command{Cmd: cmd, Run: Close}
}

func initMount() *cli.Command {
	cmd := &cobra.Command{
		Use:   "mount",
		Short: "Mounts disks. Disks must be opened.",
	}

	return &cli.Command{Cmd: cmd, Run: Mount}
}

func initOpen() *cli.Command {
	cmd := &cobra.Command{
		Use:   "open",
		Short: "Opens encrypted disks.",
	}

	return &cli.Command{Cmd: cmd, Run: Open}
}

func initUnmount() *cli.Command {
	cmd := &cobra.Command{
		Use:   "unmount",
		Short: "Unmounts disks. Services on fs must be stopped.",
	}

	return &cli.Command{Cmd: cmd, Run: Unmount}
}

func initStart() *cli.Command {
	cmd := &cobra.Command{
		Use:   "start",
		Short: "Runs `open` and `mount` consistently.",
	}

	return &cli.Command{Cmd: cmd, Run: Start}
}

func initStop() *cli.Command {
	cmd := &cobra.Command{
		Use:   "stop",
		Short: "Runs `unmount` and `close` consistently.",
	}

	return &cli.Command{Cmd: cmd, Run: Stop}
}

func initFormat() *cli.Command {
	cmd := &cobra.Command{
		Use:   "format-all-data",
		Short: "Formats disks. All data will be lost.",
	}

	return &cli.Command{Cmd: cmd, Run: Format}
}

func mustDisklock(env *cli.Env) *disklock.Disklock {
	err := config.LoadFromAbsolutePath(env.GetConfigPath(), &cfg)
	if err != nil {
		env.L().Fatal("can not read config file", log.Error(err), log.String("configFile", env.GetConfigPath()))
	}
	dl, err := disklock.NewDisklock(cfg.Disklock, env.App)
	if err != nil {
		env.L().Fatal("can not create disklock", log.Error(err))
	}
	return dl
}

func mustRun(env *cli.Env, f func() error) {
	if err := f(); err != nil {
		env.L().Fatal("got an error", log.Error(err))
	}
}

func Close(_ context.Context, env *cli.Env, _ *cobra.Command, _ []string) {
	dl := mustDisklock(env)
	mustRun(env, dl.Close)
}

func Mount(_ context.Context, env *cli.Env, _ *cobra.Command, _ []string) {
	dl := mustDisklock(env)
	mustRun(env, dl.Mount)
}

func Open(_ context.Context, env *cli.Env, _ *cobra.Command, _ []string) {
	dl := mustDisklock(env)
	mustRun(env, dl.Open)
}

func Unmount(_ context.Context, env *cli.Env, _ *cobra.Command, _ []string) {
	dl := mustDisklock(env)
	mustRun(env, dl.Unmount)
}

func Start(_ context.Context, env *cli.Env, _ *cobra.Command, _ []string) {
	dl := mustDisklock(env)
	err := dl.Open()
	if err == nil {
		err = dl.Mount()
	}

	if err != nil {
		env.L().Fatal("got an error", log.Error(err))
	}
}

func Stop(_ context.Context, env *cli.Env, _ *cobra.Command, _ []string) {
	dl := mustDisklock(env)
	err := dl.Unmount()
	if err == nil {
		err = dl.Close()
	}

	if err != nil {
		env.L().Fatal("got an error", log.Error(err))
	}
}

func Format(_ context.Context, env *cli.Env, _ *cobra.Command, _ []string) {
	dl := mustDisklock(env)
	mustRun(env, dl.Format)
}

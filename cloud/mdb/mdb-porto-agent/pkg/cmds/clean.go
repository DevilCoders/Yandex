package cmds

import (
	"context"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/config"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/statestore/fs"
)

var (
	cmdClean = initClean()
)

const (
	defaultHistTTL = 14 * 24 * time.Hour
)

func initClean() *cli.Command {
	cmd := &cobra.Command{
		Use:   "clean obsolete history of containers",
		Short: "clean obsolete history of containers",
		Long:  "clean obsolete history of containers",
	}

	return &cli.Command{Cmd: cmd, Run: clean}
}

// clean porto state history
func clean(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	conf := config.FromEnv(env)
	ss, err := fs.New(conf.StoreCachePath, conf.StoreLibPath, env.Logger)
	if err != nil {
		env.Logger.Fatalf("failed to create statestore: %s", err)
	}

	lc, err := ss.ListHistoryContainers()
	if err != nil {
		env.Logger.Fatalf("failed to get list of containers in history: %s", err)
	}

	env.Logger.Debugf("list of containers: %v", lc)
	for _, c := range lc {
		ss.CleanHistory(env.IsDryRunMode(), c, defaultHistTTL)
	}
}

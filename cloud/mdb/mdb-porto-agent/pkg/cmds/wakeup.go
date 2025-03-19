package cmds

import (
	"context"
	"os"
	"time"

	"github.com/cenkalti/backoff/v4"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/config"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/dns"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/porto"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/porto/network"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/porto/runner"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/statestore/fs"
	portoapi "a.yandex-team.ru/infra/tcp-sampler/pkg/porto"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	cmdWakeup = initWakeup()

	flagUnexpected bool
)

const (
	exitCodeUnexpectedChanges = 13
	maxTryPortoAPIConnectTime = 3 * time.Minute

	flagNameUnexpected      = "unexpected"
	flagNameShortUnexpected = "u"
)

func initWakeup() *cli.Command {
	cmd := &cobra.Command{
		Use:   "wake-up all containers by cached states",
		Short: "wake-up all containers by cached states",
		Long:  "wake-up porto state for each known container by cached states",
	}

	cmd.Flags().BoolVarP(
		&flagUnexpected,
		flagNameUnexpected,
		flagNameShortUnexpected,
		false,
		"on unexpected changes returns an error",
	)

	return &cli.Command{Cmd: cmd, Run: wakeup}
}

func portoapiWaitConnect(l log.Logger) (portoapi.API, error) {
	bo := backoff.NewExponentialBackOff()
	bo.MaxElapsedTime = maxTryPortoAPIConnectTime
	for {
		api, err := portoapi.Connect()
		if err == nil {
			return api, nil
		}
		d := bo.NextBackOff()
		if d == backoff.Stop {
			return nil, porto.ErrPortoAPIFailed.Wrap(err)
		}
		l.Warnf("failed to connect porto API and sleep for next try %s, error: %v", d, err)
		time.Sleep(d)
	}
}

// wakeup porto state for all available contaner states in cache
func wakeup(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	conf := config.FromEnv(env)
	ss, err := fs.New(conf.StoreCachePath, conf.StoreLibPath, env.Logger)
	if err != nil {
		env.Logger.Fatalf("failed to create statestore: %v", err)
	}

	lc, err := ss.ListContainers()
	if err != nil {
		env.Logger.Fatalf("failed to list statestore cache: %v", err)
	}
	if len(lc) == 0 {
		env.Logger.Infof("no container states at all, nothing to wake up")
		return
	}
	env.Logger.Infof("list of containers: %v", lc)

	api, err := portoapiWaitConnect(env.Logger)
	if err != nil {
		env.Logger.Fatalf("failed to connect porto API: %v", err)
	}

	net := network.New(env.Logger)
	ps, err := porto.New(env.Logger, env.IsDryRunMode(), api, net)
	if err != nil {
		env.Logger.Fatalf("failed to create porto support: %v", err)
	}

	pr := runner.New(env.Logger)

	changeCount := 0
	for _, c := range lc {
		container, st, err := ss.GetTargetState(c)
		if err != nil {
			env.Logger.Errorf("error on load container %s state: %v", c, err)
			continue
		}
		uctx := updateCtx{
			log:       env.Logger,
			container: container,
			drm:       env.IsDryRunMode(),
		}
		changes, err := updateContainer(ctx, uctx, ps, pr, ss, secrets{}, st, conf.DbmURL, dns.NewDefaultResolver(), resolveWaitTimeout, resolveWaitPeriod)
		if err != nil {
			env.Logger.Errorf("error on update container %s: %v", c, err)
		}
		changeCount += len(changes)
	}
	if flagUnexpected && changeCount > 0 {
		env.Logger.Errorf("got %d unexpected chages", changeCount)
		os.Exit(exitCodeUnexpectedChanges)
	}
}

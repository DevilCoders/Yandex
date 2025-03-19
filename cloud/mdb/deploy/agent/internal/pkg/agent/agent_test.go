package agent_test

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent"
	agentmocks "a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/mocks"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/salt"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/commander"
	commandermock "a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/commander/mocks"
	datasourcemock "a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/datasource/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestAgent(t *testing.T) {
	lg, err := zap.New(zap.CLIConfig(log.DebugLevel))
	require.NoError(t, err)

	for _, tt := range []struct {
		name     string
		commands []commander.Command
	}{
		{
			"Run one command",
			[]commander.Command{
				{
					ID:   "t1",
					Name: "test.nop",
				},
			},
		},
		{
			"Run two command",
			[]commander.Command{
				{
					ID:   "t1",
					Name: "test.nop",
				},
				{
					ID:   "t2",
					Name: "test.nop",
				},
			},
		},
		{
			"Run two conflicting commands",
			[]commander.Command{
				{
					ID:   "h1",
					Name: "state.highstate",
				},
				{
					ID:   "h2",
					Name: "state.highstate",
				},
			},
		},
	} {

		t.Run(tt.name, func(t *testing.T) {
			ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
			defer cancel()
			mockCtrl := gomock.NewController(t)
			defer mockCtrl.Finish()

			commanderMock := commandermock.NewMockCommandSourcer(mockCtrl)
			cmds := make(chan commander.Command, len(tt.commands))
			commanderMock.EXPECT().Commands(gomock.Any()).AnyTimes().Return(cmds)

			dataSource := datasourcemock.NewMockDataSource(mockCtrl)
			dataSource.EXPECT().LatestVersion(gomock.Any()).AnyTimes().Return("trunk", nil).AnyTimes()
			srvManager := agentmocks.NewMockSrvManager(mockCtrl)
			srvManager.EXPECT().Version().Return("trunk", nil).AnyTimes()

			callManager := agentmocks.NewMockCallManager(mockCtrl)
			changes := make(chan salt.Change, len(tt.commands))
			callManager.EXPECT().Changes().AnyTimes().Return(changes)
			callManager.EXPECT().Run(gomock.Any(), gomock.Any(), false).Do(func(_, arg1, _ interface{}) {
				job := arg1.(salt.Job)
				go func() {
					changes <- salt.Change{
						Job:       job,
						PID:       42,
						StartedAt: time.Now(),
						Result: salt.Result{
							ExitCode:   0,
							FinishedAt: time.Now(),
						},
					}
				}()
			}).AnyTimes()

			ag := agent.New(agent.DefaultConfig(), lg, commanderMock, dataSource, callManager, srvManager)

			res := make([]commander.Result, 0, len(tt.commands))
			shutdown := make(chan struct{})
			commanderMock.EXPECT().Done(gomock.Any(), gomock.Any()).Do(func(_, arg1 interface{}) {
				r := arg1.(commander.Result)
				res = append(res, r)
				if len(res) == len(tt.commands) {
					go func() {
						close(shutdown)
					}()
				}
			}).Times(len(tt.commands))

			for _, c := range tt.commands {
				cmds <- c
			}

			ag.Run(ctx, shutdown)

			require.Len(t, res, len(tt.commands))
		})
	}

	t.Run("Run command with specified version and update srv", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
		defer cancel()
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()

		commanderMock := commandermock.NewMockCommandSourcer(mockCtrl)
		cmds := make(chan commander.Command, 1)
		commanderMock.EXPECT().Commands(gomock.Any()).AnyTimes().Return(cmds)

		dataSource := datasourcemock.NewMockDataSource(mockCtrl)
		dataSource.EXPECT().ResolveVersion(gomock.Any(), "unk").Return("100500-trunk", nil)
		dataSource.EXPECT().Fetch(gomock.Any(), "100500-trunk").Return(nil, nil)
		srvManager := agentmocks.NewMockSrvManager(mockCtrl)
		srvManager.EXPECT().Version().Return("trunk", nil)
		srvManager.EXPECT().Update(nil).Return(nil)

		callManager := agentmocks.NewMockCallManager(mockCtrl)
		changes := make(chan salt.Change, 1)
		callManager.EXPECT().Changes().AnyTimes().Return(changes)
		callManager.EXPECT().Run(gomock.Any(), gomock.Any(), false).Do(func(_, arg1, _ interface{}) {
			job := arg1.(salt.Job)
			changes <- salt.Change{
				Job:       job,
				PID:       42,
				StartedAt: time.Now(),
				Result: salt.Result{
					ExitCode:   0,
					FinishedAt: time.Now(),
				},
			}
		})

		ag := agent.New(agent.DefaultConfig(), lg, commanderMock, dataSource, callManager, srvManager)

		shutdown := make(chan struct{})
		commanderMock.EXPECT().Done(gomock.Any(), gomock.Any()).Do(func(_, arg1 interface{}) {
			close(shutdown)
		})
		cmds <- commander.Command{ID: "t1", Name: "test.nop", Source: commander.DataSource{Version: "unk"}}

		ag.Run(ctx, shutdown)
	})

	t.Run("Shutdown while have running commands", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
		defer cancel()
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()

		commanderMock := commandermock.NewMockCommandSourcer(mockCtrl)
		cmds := make(chan commander.Command, 2)
		commanderMock.EXPECT().Commands(gomock.Any()).AnyTimes().Return(cmds)
		commanderMock.EXPECT().Done(gomock.Any(), gomock.Any()).Return()

		dataSource := datasourcemock.NewMockDataSource(mockCtrl)
		dataSource.EXPECT().LatestVersion(gomock.Any()).AnyTimes().Return("trunk", nil).AnyTimes()
		srvManager := agentmocks.NewMockSrvManager(mockCtrl)
		srvManager.EXPECT().Version().Return("trunk", nil).AnyTimes()

		callManager := agentmocks.NewMockCallManager(mockCtrl)
		changes := make(chan salt.Change, 2)
		callManager.EXPECT().Changes().AnyTimes().Return(changes)

		cmdStarted := make(chan struct{})
		callManager.EXPECT().Run(gomock.Any(), gomock.Any(), false).Do(func(_, _, _ interface{}) {
			close(cmdStarted)
		})
		callManager.EXPECT().Shutdown().Do(func() {
			changes <- salt.Change{
				Job:       salt.Job{ID: "t1"},
				PID:       42,
				StartedAt: time.Now(),
				Result: salt.Result{
					ExitCode:   -1,
					FinishedAt: time.Now(),
					Error:      xerrors.New("killed"),
				},
			}
		})

		cfg := agent.DefaultConfig()
		cfg.ShutdownTimeout.Duration = 1
		ag := agent.New(cfg, lg, commanderMock, dataSource, callManager, srvManager)
		shutdown := make(chan struct{})

		cmds <- commander.Command{ID: "t1", Name: "test.nop"}
		runDone := make(chan struct{})
		go func() {
			ag.Run(ctx, shutdown)
			close(runDone)
		}()
		<-cmdStarted
		close(shutdown)
		<-runDone
	})

	t.Run("Run command with progress", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
		defer cancel()
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()

		commanderMock := commandermock.NewMockProgressTrackedCommandSourcer(mockCtrl)
		cmds := make(chan commander.Command, 1)
		commanderMock.EXPECT().Commands(gomock.Any()).AnyTimes().Return(cmds)

		dataSource := datasourcemock.NewMockDataSource(mockCtrl)
		dataSource.EXPECT().LatestVersion(gomock.Any()).AnyTimes().Return("trunk", nil).AnyTimes()
		srvManager := agentmocks.NewMockSrvManager(mockCtrl)
		srvManager.EXPECT().Version().Return("trunk", nil).AnyTimes()

		callManager := agentmocks.NewMockCallManager(mockCtrl)
		changes := make(chan salt.Change, 100)
		callManager.EXPECT().Changes().AnyTimes().Return(changes)
		callManager.EXPECT().Run(gomock.Any(), gomock.Any(), true).Do(func(_, arg1, _ interface{}) {
			job := arg1.(salt.Job)
			go func() {
				changes <- salt.Change{
					Job:       job,
					PID:       42,
					StartedAt: time.Now(),
					Progress:  "It did some job",
				}
				changes <- salt.Change{
					Job:       job,
					PID:       42,
					StartedAt: time.Now(),
					Result: salt.Result{
						ExitCode:   0,
						FinishedAt: time.Now(),
					},
				}
			}()
		}).AnyTimes()

		ag := agent.New(agent.DefaultConfig(), lg, commanderMock, dataSource, callManager, srvManager)

		res := make([]commander.Result, 0, 1)
		shutdown := make(chan struct{})
		commanderMock.EXPECT().Track(gomock.Any(), gomock.Any()).MinTimes(2)
		commanderMock.EXPECT().Done(gomock.Any(), gomock.Any()).Do(func(_, arg1 interface{}) {
			r := arg1.(commander.Result)
			res = append(res, r)
			if len(res) == 1 {
				go func() {
					close(shutdown)
				}()
			}
		})

		cmds <- commander.Command{ID: "t1", Name: "test.nop", Options: commander.Options{Progress: true}}
		runDone := make(chan struct{})
		go func() {
			ag.Run(ctx, shutdown)
			close(runDone)
		}()
		<-runDone
	})
}

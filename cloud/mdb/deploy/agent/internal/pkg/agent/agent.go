package agent

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/salt"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/commander"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/datasource"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	MaxHistory     int                   `json:"max_history" yaml:"max_history"`
	HistoryKeepAge encodingutil.Duration `json:"history_keep_age" yaml:"history_keep_age"`
	Salt           salt.Config           `json:"salt" yaml:"salt"`

	ShutdownTimeout encodingutil.Duration `json:"shutdown_timeout" yaml:"shutdown_timeout"`
}

func DefaultConfig() Config {
	return Config{
		MaxHistory:      500,
		HistoryKeepAge:  encodingutil.FromDuration(time.Minute * 5),
		Salt:            salt.DefaultConfig(),
		ShutdownTimeout: encodingutil.FromDuration(time.Hour),
	}
}

// Agent is core deploy-agent logic
// It:
//	- listen for new commands
//	- run then updating `/srv` if need it
//	- notify `CommandSourcer` on commands execution status
type Agent struct {
	cfg           Config
	commandSource commander.CommandSourcer
	dataSource    datasource.DataSource
	callManager   CallManager
	srvManager    SrvManager
	running       map[string]commander.Command
	history       map[string]time.Time
	backlog       []commander.Command
	l             log.Logger
}

func New(cfg Config, l log.Logger, source commander.CommandSourcer, dataSource datasource.DataSource, callManager CallManager, srvManager SrvManager) *Agent {
	return &Agent{
		cfg:           cfg,
		commandSource: source,
		dataSource:    dataSource,
		callManager:   callManager,
		srvManager:    srvManager,
		running:       make(map[string]commander.Command, 3),
		history:       make(map[string]time.Time, 100),
		l:             l,
	}
}

func (a *Agent) mayRunCommand(cmd commander.Command) bool {
	running := make([]commander.Command, 0, len(a.running))
	for _, r := range a.running {
		running = append(running, r)
	}
	return !salt.Conflict(cmd, running)
}

func (a *Agent) runningCommandsCount() int {
	return len(a.running)
}

func (a *Agent) runningCommandsVersion() string {
	for _, r := range a.running {
		return r.Source.Version
	}
	return ""
}

func (a *Agent) commandAlreadyExists(cmd commander.Command) error {
	if existedCommand, ok := a.running[cmd.ID]; ok {
		return fmt.Errorf("command with id %q already running: %v, new: %v. Ignore it", cmd.ID, existedCommand, cmd)
	}
	if finishedAt, ok := a.history[cmd.ID]; ok {
		return fmt.Errorf("command with id %q already finished at %s", cmd.ID, finishedAt)
	}
	return nil
}

func (a *Agent) trackProgress(ctx context.Context, commandID string, message string) {
	if progressTracker, ok := a.commandSource.(commander.ProgressTracker); ok {
		progressTracker.Track(ctx, commander.Progress{
			CommandID: commandID,
			Message:   message,
		})
	}
}

func (a *Agent) progressInfof(ctx context.Context, cmd commander.Command, format string, args ...interface{}) {
	a.l.Infof(format, args...)
	if cmd.Options.Progress {
		a.trackProgress(ctx, cmd.ID, fmt.Sprintf(format, args...))
	}
}

func (a *Agent) progressWarnf(ctx context.Context, cmd commander.Command, format string, args ...interface{}) {
	a.l.Warnf(format, args...)
	if cmd.Options.Progress {
		a.trackProgress(ctx, cmd.ID, fmt.Sprintf(format, args...))
	}
}

func (a *Agent) updateSrv(ctx context.Context, cmd commander.Command) (string, error) {
	expectedVersion := cmd.Source.Version
	if expectedVersion == "" {
		expectedVersion = a.runningCommandsVersion()
		if expectedVersion == "" {
			a.progressInfof(ctx, cmd, "resolving latest srv version, cause command doesn't specify it explicitly and there are no running commands")
			latestVersion, err := a.dataSource.LatestVersion(ctx)
			if err != nil {
				return "", xerrors.Errorf("resolve latest srv version (command doesn't specify it explicitly): %w", err)
			}
			expectedVersion = latestVersion
		}
	} else {
		// The user can pass the partial version definition: `r100500`.
		// It's better to resolve it because we will compare it with currently running that is absolute.
		resolvedVersion, err := a.dataSource.ResolveVersion(ctx, expectedVersion)
		if err != nil {
			return "", xerrors.Errorf("resolve command srv version ('%s'): %w", expectedVersion, err)
		}
		expectedVersion = resolvedVersion
	}

	currentVersion, err := a.srvManager.Version()
	if err != nil {
		switch {
		case xerrors.Is(err, ErrSrvNotInitialized):
			a.progressInfof(ctx, cmd, "Probably it's our first run: %s. We should update /srv", err)
		case xerrors.Is(err, ErrSrvMalformed):
			a.progressWarnf(ctx, cmd, "salt /srv malformed: %s. We will try update /srv so that update should fix the problem", err)
		default:
			return "", xerrors.Errorf("resolve current srv version: %w", err)
		}
	}

	if currentVersion == expectedVersion {
		a.progressInfof(ctx, cmd, "Don't need to update srv, cause it's match required version: %s", expectedVersion)
		return expectedVersion, nil
	}

	a.progressInfof(ctx, cmd, "updating srv '%s' -> '%s'", currentVersion, expectedVersion)
	image, err := a.dataSource.Fetch(ctx, expectedVersion)
	if err != nil {
		return "", xerrors.Errorf("fetch %s srv: %w", expectedVersion, err)
	}
	if err := a.srvManager.Update(image); err != nil {
		return "", xerrors.Errorf("updating /srv '%s' -> '%s': %w", currentVersion, expectedVersion, err)
	}
	return expectedVersion, nil
}

func (a *Agent) runCommand(ctx context.Context, cmd commander.Command) {
	if err := a.commandAlreadyExists(cmd); err != nil {
		a.l.Infof("duplicate command: %s", err)
		a.commandSource.Done(ctx, commander.Result{
			CommandID: cmd.ID,
			ExitCode:  -1,
			Error:     err,
		})
		return
	}

	job, err := salt.FormCommand(a.cfg.Salt, cmd)
	if err != nil {
		a.l.Infof("invalid command: %s", err)
		a.commandSource.Done(ctx, commander.Result{
			CommandID: cmd.ID,
			ExitCode:  -1,
			Error:     err,
		})
		return
	}

	updatedToVersion, err := a.updateSrv(ctx, cmd)
	if err != nil {
		a.l.Errorf("failed to update srv: %s", err)
		a.commandSource.Done(ctx, commander.Result{
			CommandID: cmd.ID,
			ExitCode:  -1,
			Error:     err,
		})
		return
	}
	cmd.Source.Version = updatedToVersion

	a.l.Debugf("running new command: %+v", cmd)

	a.running[cmd.ID] = cmd
	a.callManager.Run(ctx, job, cmd.Options.Progress)
}

func (a *Agent) onJobDone(ctx context.Context, change salt.Change) {
	a.l.Debugf("job %+v is done", change.Job)
	a.commandSource.Done(ctx, commander.Result{
		CommandID:  change.Job.ID,
		StartedAt:  change.StartedAt,
		ExitCode:   change.Result.ExitCode,
		Error:      change.Result.Error,
		Stdout:     change.Result.Stdout,
		Stderr:     change.Result.Stderr,
		FinishedAt: change.Result.FinishedAt,
	})

	if _, ok := a.running[change.Job.ID]; ok {
		delete(a.running, change.Job.ID)
	} else {
		a.l.Warnf("Got done job change %+v that not present in running commandsTotal %+v", change, a.running)
	}
	a.history[change.Job.ID] = change.Result.FinishedAt
	a.cleanupHistory()
}

func (a *Agent) cleanupHistory() {
	if len(a.history) >= a.cfg.MaxHistory {
		oldRecords := time.Now().Add(-a.cfg.HistoryKeepAge.Duration)
		a.l.Debugf("Cleanup history recorde before %s, cause it contains %d items", oldRecords, len(a.history))
		for jid, finishedAt := range a.history {
			if finishedAt.Before(oldRecords) {
				delete(a.history, jid)
			}
		}
		a.l.Debugf("After cleanup history contains %d items", len(a.history))
	}
}

func (a *Agent) addToBacklog(cmd commander.Command) {
	a.backlog = append(a.backlog, cmd)
	metrics.backlogSize.Set(float64(len(a.backlog)))
}

func (a *Agent) examineBacklog(ctx context.Context) {
	if len(a.backlog) > 0 {
		var newBacklog []commander.Command
		for _, cmd := range a.backlog {
			if a.mayRunCommand(cmd) {
				a.runCommand(ctx, cmd)
			} else {
				newBacklog = append(newBacklog, cmd)
			}
		}
		a.l.Debugf("After backlog process there are %d commandsTotal in it", len(newBacklog))
		a.backlog = newBacklog
		metrics.backlogSize.Set(float64(len(a.backlog)))
	}
}

func (a *Agent) Run(ctx context.Context, shutdown <-chan struct{}) {
	var shutdownInProgress bool
	var shutdownCommandsCalled bool
	// Not intuitive code, but I need a timer that starts after the shutdown request.
	// Here we use that receiving from nil channel is allowed, so we can use Timer.C
	shutdownDeadline := &time.Timer{}

	for {
		if shutdownInProgress {
			if runningCount := a.runningCommandsCount(); runningCount == 0 {
				a.l.Info("There are no running commands, so we may simply finish")
				return
			}
		}

		select {
		case <-ctx.Done():
			a.l.Info("Context done. Exiting from Run loop")
			return
		case <-shutdown:
			shutdownInProgress = true
			shutdownDeadline = time.NewTimer(a.cfg.ShutdownTimeout.Duration)
		case <-shutdownDeadline.C:
			// Sometimes the Timer can trigger more than once
			// https://paste.yandex-team.ru/7535368
			// Explanation: https://stackoverflow.com/a/71191284
			if !shutdownCommandsCalled {
				a.l.Infof("%s timeout for a clean shutdown is expired", a.cfg.ShutdownTimeout.Duration)
				if runningCount := a.runningCommandsCount(); runningCount != 0 {
					a.l.Warnf("There are %d commands running. Shutdown them gracefully", runningCount)
					a.callManager.Shutdown()
					shutdownCommandsCalled = true
				}
			}
		case change := <-a.callManager.Changes():
			if change.Result.Done() {
				metrics.doneCommands.Inc()
				a.onJobDone(ctx, change)
				a.examineBacklog(ctx)
			} else {
				a.trackProgress(ctx, change.Job.ID, change.Progress)
			}
		case cmd := <-a.commandSource.Commands(ctx):
			if shutdownInProgress {
				a.l.Warnf("Ignore new command %+v. Cause shutdown in progress", cmd)
				continue
			}
			metrics.commandsTotal.Inc()
			if a.mayRunCommand(cmd) {
				a.runCommand(ctx, cmd)
			} else {
				a.l.Infof("Put %+v to backlog, cause shouldn't start it at that moment", cmd)
				a.addToBacklog(cmd)
			}
		}
	}
}

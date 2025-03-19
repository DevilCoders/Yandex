package core

import (
	"context"
	"fmt"
	"sync/atomic"
	"time"

	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/deploy/internal/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi/cherrypy"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type SaltAPITimeoutsConfig struct {
	FindJob   time.Duration `json:"find_job" yaml:"find_job"`
	Running   time.Duration `json:"running" yaml:"running"`
	IsRunning time.Duration `json:"is_running" yaml:"is_running"`
	TestPing  time.Duration `json:"test_ping" yaml:"test_ping"`
	AsyncRun  time.Duration `json:"async_run" yaml:"async_run"`
	Config    time.Duration `json:"config" yaml:"config"`
}

func DefaultSaltAPITimeoutsConfig() SaltAPITimeoutsConfig {
	return SaltAPITimeoutsConfig{
		FindJob:   15 * time.Second,
		Running:   15 * time.Second,
		IsRunning: 15 * time.Second,
		TestPing:  15 * time.Second,
		AsyncRun:  15 * time.Second,
		Config:    time.Minute,
	}
}

type knownMaster struct {
	// Info is how deploy system views this master (cached value of master from deploy db)
	info atomic.Value

	fqdn string
	// Our view of this master (did we ping it successfully or not)
	accessible    int32
	workerThreads int32 // Caches a count of workers on master
	client        saltapi.Client
	auth          saltapi.Auth
	cancel        context.CancelFunc

	checkerName       string
	checkPeriod       time.Duration
	failCount         int
	dispatchPeriod    time.Duration
	maxTasksPerThread int32

	runningJobCheckConfig RunningJobCheckConfig
	saltAPITimeoutsConfig SaltAPITimeoutsConfig

	ddb deploydb.Backend
	lg  log.Logger
}

func newKnownMaster(ctx context.Context, m models.Master, addr string, cfg Config, ddb deploydb.Backend, lg log.Logger) (*knownMaster, error) {
	// Create salt api client
	client, err := cherrypy.New("https://"+addr, cfg.SaltAPI.HTTP.TLS, cfg.SaltAPI.HTTP.Logging, lg)
	if err != nil {
		return nil, err
	}

	auth := client.NewAuth(cfg.SaltAPI.Auth)

	ctx, cancel := context.WithCancel(ctx)
	ctx = ctxlog.WithFields(ctx, log.String("salt-master.fqdn", m.FQDN))

	km := &knownMaster{
		fqdn:                  m.FQDN,
		client:                client,
		auth:                  auth,
		cancel:                cancel,
		checkerName:           cfg.MasterCheckerName,
		checkPeriod:           cfg.MasterCheckPeriod,
		failCount:             cfg.MasterCheckFailCount,
		dispatchPeriod:        cfg.CommandDispatchPeriod,
		maxTasksPerThread:     cfg.MasterMaxTasksPerThread,
		runningJobCheckConfig: cfg.RunningJobCheck,
		saltAPITimeoutsConfig: cfg.SaltAPITimeouts,
		ddb:                   ddb,
		lg:                    lg,
	}

	km.info.Store(m)
	go auth.AutoAuth(ctx, cfg.SaltAPI.ReAuthPeriod, cfg.SaltAPI.AuthAttemptTimeout, lg)
	go km.backgroundChecker(ctx)
	go km.backgroundDispatcher(ctx)
	go km.backgroundRunningJobsChecker(ctx)
	return km, nil
}

func (km *knownMaster) openTracingTags() opentracing.Tags {
	return opentracing.Tags{
		"master_fqdn": km.fqdn,
	}
}

func (km *knownMaster) StoreInfo(m models.Master) {
	km.info.Store(m)
}

type masterStatus int32

func (s masterStatus) String() string {
	switch s {
	case masterStatusUnknown:
		return "unknown"
	case masterStatusDead:
		return "dead"
	case masterStatusAlive:
		return "alive"
	default:
		panic(fmt.Sprintf("unknown accessible value %d", s))
	}
}

const (
	masterStatusUnknown masterStatus = iota
	masterStatusDead
	masterStatusAlive
)

func (km *knownMaster) setAccessible(ctx context.Context, alive bool) (masterStatus, masterStatus, bool) {
	accessible := masterStatusDead
	if alive {
		accessible = masterStatusAlive
	}

	old := masterStatus(atomic.SwapInt32(&km.accessible, int32(accessible)))

	if _, err := km.ddb.UpdateMasterCheck(ctx, km.fqdn, km.checkerName, alive); err != nil {
		ctxlog.Errorf(ctx, km.lg, "Failed to update master check (%t) for %q: %s", alive, km.fqdn, err)
	}

	return accessible, old, old != accessible
}

func (km *knownMaster) isAccessible() bool {
	return masterStatus(atomic.LoadInt32(&km.accessible)) == masterStatusAlive
}

func (km *knownMaster) backgroundChecker(ctx context.Context) {
	ctx = ctxlog.WithFields(ctx, log.String("fqdn", km.fqdn))
	ctxlog.Debugf(ctx, km.lg, "Started alive checker for %q", km.fqdn)
	defer ctxlog.Debugf(ctx, km.lg, "Stopped alive checker for %q", km.fqdn)

	timer := time.NewTimer(0)

	for {
		select {
		case <-ctx.Done():
			if !timer.Stop() {
				<-timer.C
			}
			return
		case <-timer.C:
			km.check(ctx)
			timer.Reset(km.checkPeriod)
		}
	}
}

func (km *knownMaster) check(ctx context.Context) {
	timer := time.NewTimer(0)
	attempt := 1

	span, ctx := opentracing.StartSpanFromContext(ctx, "Background CheckSaltMaster", km.openTracingTags())
	origCtx := ctx
	defer span.Finish()

	for {
		var timeoutSpan opentracing.Span
		if attempt > 1 {
			// Start timeout span
			// Must not use returned ctx as it has timeout span which should not be used for anything
			timeoutSpan, _ = opentracing.StartSpanFromContext(ctx, "Timeout", km.openTracingTags())
		}

		select {
		case <-ctx.Done():
			if timeoutSpan != nil {
				timeoutSpan.Finish()
			}

			if !timer.Stop() {
				<-timer.C
			}

			// We definitely failed to check this master
			ext.Error.Set(span, true)
			return
		case <-timer.C:
			ctx = ctxlog.WithFields(origCtx, log.Int("attempt", attempt))
			if timeoutSpan != nil {
				timeoutSpan.Finish()
			}

			ctxlog.Debugf(ctx, km.lg, "Verifying salt master %q: attempt number %d", km.fqdn, attempt)

			err := km.doPing(ctx)
			if err == nil {
				ctxlog.Debugf(ctx, km.lg, "Salt master %q is %s and well", km.fqdn, masterStatusAlive)
				wt, err := km.client.Config().WorkerThreads(ctx, km.auth, km.saltAPITimeoutsConfig.Config)
				if err == nil {
					km.workerThreads = wt
					current, old, ok := km.setAccessible(ctx, true)
					if ok {
						ctxlog.Warnf(ctx, km.lg, "Salt master %q went from %s to %s", km.fqdn, old, current)
					}
					return
				}

				// Consider salt-master dead if we can't retrieve information about its configuration.
				// This can be the case when authentication fails or due to some other misconfiguration.
				ctxlog.Errorf(ctx, km.lg,
					"Failed to get master worker thread count for %q, pinged %d times (%d fails is considered fatal): %s",
					km.fqdn, attempt, km.failCount, err)
			} else {
				ctxlog.Errorf(ctx, km.lg,
					"Failed to ping salt master %q %d times (%d fails is considered fatal): %s",
					km.fqdn, attempt, km.failCount, err,
				)
			}

			attempt++
			if attempt > km.failCount {
				current, old, ok := km.setAccessible(ctx, false)
				if ok {
					ctxlog.Warnf(ctx, km.lg, "Salt master %q went from %s to %s, we failed to ping it %d times", km.fqdn, old, current, km.failCount)
				} else {
					ctxlog.Infof(ctx, km.lg, "Salt master %q is still considered %s, we failed to ping it %d times", km.fqdn, current, km.failCount)
				}
				ext.Error.Set(span, true)
				return
			}

			timer.Reset(time.Second)
		}
	}
}

func (km *knownMaster) doPing(ctx context.Context) error {
	ctx, cancel := context.WithTimeout(ctx, time.Second*10)
	defer cancel()
	return km.client.Ping(ctx, km.auth)
}

func (km *knownMaster) backgroundDispatcher(ctx context.Context) {
	ctxlog.Debugf(ctx, km.lg, "Started command dispatcher for %q", km.fqdn)
	defer ctxlog.Debugf(ctx, km.lg, "Stopped command dispatcher for %q", km.fqdn)

	timer := time.NewTimer(km.dispatchPeriod)

	for {
		select {
		case <-ctx.Done():
			if !timer.Stop() {
				<-timer.C
			}
			return
		case <-timer.C:
			if !km.isAccessible() {
				ctxlog.Debugf(ctx, km.lg, "Not dispatching commands for %q master - its dead from our point of view", km.fqdn)
				timer.Reset(km.dispatchPeriod)
				continue
			}

			if !km.dispatchCommands(ctx) {
				timer.Reset(km.dispatchPeriod)
				continue
			}

			timer.Reset(time.Nanosecond)
		}
	}
}

func (km *knownMaster) dispatchCommands(ctx context.Context) bool {
	ctxlog.Debugf(ctx, km.lg, "Retrieving commands for master %s", km.fqdn)

	span, ctx := opentracing.StartSpanFromContext(ctx, "Background DispatchCommands", km.openTracingTags())
	defer span.Finish()

	// Use small limit of commands to dispatch to better control a throttling mode
	cd, err := km.ddb.CommandsForDispatch(ctx, km.fqdn, 1, km.workerThreads*km.maxTasksPerThread)
	if err != nil {
		if semerr.IsNotFound(err) {
			tags.DispatchCommandsCount.Set(span, 0)
			ctxlog.Debugf(ctx, km.lg, "No commands to dispatch for master %s", km.fqdn)
		} else {
			ctxlog.Errorf(ctx, km.lg, "Failed to retrieve commands for dispatch: %s", err)
		}

		return false
	}
	defer func() { _ = cd.Close(ctx) }()

	tags.DispatchCommandsCount.Set(span, len(cd.Commands()))

	if len(cd.Commands()) == 0 {
		ctxlog.Debugf(ctx, km.lg, "No commands to dispatch for master %s", km.fqdn)
		return false
	}

	for _, cmd := range cd.Commands() {
		km.dispatchCommand(ctx, cd, cmd)
	}

	return true
}

func (km *knownMaster) dispatchCommand(ctx context.Context, cd deploydb.CommandDispatcher, cmd models.Command) {
	ctxlog.Debugf(ctx, km.lg, "Dispatching command %+v on master %s", cmd, km.fqdn)

	span, ctx := opentracing.StartSpanFromContext(ctx, "Command Dispatch", km.openTracingTags())
	defer span.Finish()

	tags.ShipmentID.Set(span, cmd.ShipmentID)
	tags.CommandID.Set(span, cmd.ID)
	tags.MinionFQDN.Set(span, cmd.FQDN)

	// Ping minion before running command - minion might be dead or unresponsive
	// or master might not have accepted its key yet or some command run right now
	if !km.canRun(ctx, cmd) {
		if _, err := cd.DispatchFailed(ctx, cmd.ID); err != nil {
			ctxlog.Errorf(ctx, km.lg, "Failed to mark dispatch failure for command %+v: %s", cmd, err)
			tracing.SetErrorOnSpan(span, err)
		}

		return
	}

	jid, err := km.client.AsyncRun(ctx, km.auth, km.saltAPITimeoutsConfig.AsyncRun, cmd.FQDN, cmd.Type, cmd.Args...)
	if err != nil {
		ctxlog.Errorf(ctx, km.lg, "Failed execute command %+v: %s", cmd.ID, err)
		if _, err = cd.DispatchFailed(ctx, cmd.ID); err != nil {
			ctxlog.Errorf(ctx, km.lg, "Failed to mark dispatch failure for command %+v: %s", cmd, err)
			tracing.SetErrorOnSpan(span, err)
		}

		return
	}

	ctxlog.Infof(ctx, km.lg, "Started command %+v with jid %s", cmd, jid)
	if _, err = cd.Dispatched(ctx, cmd.ID, jid); err != nil {
		ctxlog.Errorf(ctx, km.lg, "Failed to mark command %+v as dispatched: %s", cmd, err)
		tracing.SetErrorOnSpan(span, err)
	}
}

func (km *knownMaster) pingMinion(ctx context.Context, fqdn string) bool {
	res, err := km.client.Test().Ping(ctx, km.auth, km.saltAPITimeoutsConfig.TestPing, fqdn)
	if err != nil {
		ctxlog.Errorf(ctx, km.lg, "Failed to ping %q minion: %s", fqdn, err)
		return false
	}

	v, ok := res[fqdn]
	if !ok {
		ctxlog.Errorf(ctx, km.lg, "Failed to find minion %q in ping result: %+v", fqdn, res)
		return false
	}

	if !v {
		ctxlog.Warnf(ctx, km.lg, "Ping result for minion %q is negative", fqdn)
	}

	return v
}

const (
	conflictingCommand = "state.highstate"
)

func (km *knownMaster) canRun(ctx context.Context, cmd models.Command) bool {
	if !km.pingMinion(ctx, cmd.FQDN) {
		return false
	}

	res, err := km.client.SaltUtil().IsRunning(ctx, km.auth, km.saltAPITimeoutsConfig.IsRunning, cmd.FQDN, conflictingCommand)
	if err != nil {
		ctxlog.Errorf(ctx, km.lg, "Failed to check for running conflicting command on minion %s: %s", cmd.FQDN, err)
		return false
	}

	running, ok := res[cmd.FQDN]
	if !ok {
		ctxlog.Errorf(ctx, km.lg, "Minion %q not found in result %+v", cmd.FQDN, res)
		return false
	}

	// Do we have conflicting command anywhere?
	if cmd.CommandDef.Type != conflictingCommand && len(running) == 0 {
		ctxlog.Debugf(ctx, km.lg, "Command %+v is not conflicting and no running conflicting commands found", cmd)
		return true
	}

	if len(running) != 0 {
		ctxlog.Infof(ctx, km.lg, "Minion %q has conflicting job(s) running: %+v", cmd.FQDN, running)
		return false
	}

	// If we are trying to run conflicting command, we cannot have any states running on minion
	if cmd.CommandDef.Type == conflictingCommand {
		res, err := km.client.State().Running(ctx, km.auth, km.saltAPITimeoutsConfig.Running, cmd.FQDN)
		if err != nil {
			ctxlog.Errorf(ctx, km.lg, "Failed to check for running states on minion %q: %s", cmd.FQDN, err)
			return false
		}

		running, ok := res[cmd.FQDN]
		if !ok {
			ctxlog.Errorf(ctx, km.lg, "Minion %q not found in result %+v", cmd.FQDN, res)
			return false
		}

		if len(running) != 0 {
			ctxlog.Infof(ctx, km.lg, "Minion %q has running states, cannot run conflicting command %+v: %s", cmd.FQDN, cmd, running)
			return false
		}
	}

	return true
}

func (km *knownMaster) backgroundRunningJobsChecker(ctx context.Context) {
	ctxlog.Debugf(ctx, km.lg, "Started running jobs checker for %q", km.fqdn)
	defer ctxlog.Debugf(ctx, km.lg, "Stopped running jobs checker for %q", km.fqdn)

	timer := time.NewTimer(km.runningJobCheckConfig.Period)

	for {
		select {
		case <-ctx.Done():
			if !timer.Stop() {
				<-timer.C
			}
			return
		case <-timer.C:
			if !km.isAccessible() {
				ctxlog.Debugf(ctx, km.lg, "Not checking running jobs for %q master - its dead from our point of view", km.fqdn)
				timer.Reset(km.runningJobCheckConfig.Period)
				continue
			}

			if !km.checkRunningJobs(ctx) {
				timer.Reset(km.runningJobCheckConfig.Period)
				continue
			}

			timer.Reset(time.Nanosecond)
		}
	}
}

func (km *knownMaster) checkRunningJobs(ctx context.Context) bool {
	span, ctx := opentracing.StartSpanFromContext(ctx, "Background CheckRunningJobs", km.openTracingTags())
	defer span.Finish()

	checker, err := km.ddb.RunningJobsForCheck(ctx, km.fqdn, km.runningJobCheckConfig.RunningInterval, km.runningJobCheckConfig.PageSize)
	if err != nil {
		ctxlog.Errorf(ctx, km.lg, "Failed to retrieve running jobs for master: %s", err)
		return false
	}

	if len(checker.Jobs()) == 0 {
		ctxlog.Debugf(ctx, km.lg, "No running jobs to check for master %s", km.fqdn)
		return false
	}

	ctxlog.Debugf(ctx, km.lg, "executing check for %d running jobs on master %s", len(checker.Jobs()), km.fqdn)
	g, ctx := errgroup.WithContext(ctx)
	for _, job := range checker.Jobs() {
		j := job
		g.Go(func() error { return km.checkRunningJob(ctx, checker, j) })
	}

	if err = g.Wait(); err != nil {
		ctxlog.Errorf(ctx, km.lg, "running job checks returner error: %+v", err)
		return true
	}

	ctxlog.Debugf(ctx, km.lg, "running jobs check finished for master %s", km.fqdn)
	return true
}

func (km *knownMaster) checkRunningJob(ctx context.Context, checker deploydb.RunningJobsChecker, job deploydb.RunningJob) error {
	ctxlog.Debugf(ctx, km.lg, "checking running job %+v for master %s", job, km.fqdn)

	span, ctx := opentracing.StartSpanFromContext(ctx, "Check Running Job", km.openTracingTags())
	defer span.Finish()

	tags.JobExtID.Set(span, job.ExtJobID)
	tags.MinionFQDN.Set(span, job.Minion)

	res, err := km.client.SaltUtil().FindJob(ctx, km.auth, km.saltAPITimeoutsConfig.FindJob, job.Minion, job.ExtJobID)
	if err != nil {
		tracing.SetErrorOnSpan(span, err)
		return xerrors.Errorf("failed to find running job on minion %s: %w", job.Minion, err)
	}

	running, ok := res[job.Minion]
	if !ok {
		err = xerrors.Errorf("minion %q not found in result %+v", job.Minion, res)
		tracing.SetErrorOnSpan(span, err)
		return err
	}

	if running.JobID == job.ExtJobID {
		ctxlog.Debugf(ctx, km.lg, "job %q found running on minion %q: %+v", job.ExtJobID, job.Minion, running)
		return nil
	}

	jobRes, err := checker.NotRunning(ctx, job, km.runningJobCheckConfig.FailOnCount)
	if err != nil {
		tracing.SetErrorOnSpan(span, err)
		return xerrors.Errorf("failed to mark job %+v not running: %w", job, err)
	}

	ctxlog.Infof(ctx, km.lg, "reported job %+v as not running", jobRes)
	return nil
}

package core

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/erikdubbelboer/gspt"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/utils"
)

const (
	TASK_NULL = ""
)

type Job struct {
	Id         string `json:"id"`
	Type       string `json:"type"`
	Parameters string `json:"parameters"`
}

// Task contains description and parameters to execute
type Task struct {
	g   *config.CmdGlobal
	job *Job

	CmdId string

	Core *Core
}

// Create task object for job string (assuming json)
func CreateTask(g *config.CmdGlobal, job string) (*Task, error) {
	id := "(task)"
	g.Log.Debug(fmt.Sprintf("%s %s got job string as json '%s'",
		utils.GetGPid(), id, job))
	task := &Task{g: g}
	var j Job
	if len(job) > 0 {
		if err := json.Unmarshal([]byte(job), &j); err != nil {
			return nil, errors.New(fmt.Sprintf("validation error: json unmarshal error, err:'%s'", err))
		}
	}
	task.CmdId = g.Opts.Runtime.ProgramName
	task.job = &j
	j.Id, _ = utils.Id()
	return task, nil
}

// Executing a task in the address space of calling method
func (t *Task) Exec() error {
	var err error

	gspt.SetProcTitle(fmt.Sprintf("%s: executing task '%s' on '%s'",
		t.CmdId, t.job.Id, t.job.Parameters))

	t0 := time.Now()
	t.g.Log.Debug(fmt.Sprintf("%s child: set request '%s' for execution job:'%s', parameters:'%s'",
		utils.GetGPid(), t.job.Id, t.job.Type, t.job.Parameters))

	if !utils.StringInSlice(t.job.Type, []string{
		"placeholder", "logrotate", "objects-sync",
	}) {
		return errors.New(fmt.Sprintf("validation error: job type '%s' is not implemented", t.job.Type))
	}

	// Switching between different types of jobs
	switch t.job.Type {

	case "placeholder":
		// Placeholder task just to test some actions
		// or events, e.g. listing RIB table each minute
		// from bgp session established
		err = t.ExecPlaceholder()

	case "logrotate":
		var LogrotateOptions config.LogrotateOptions
		LogrotateOptions.Move = true
		err = t.Logrotate(&LogrotateOptions)

	case "objects-sync":

		// objects syncing
		var Timeout int
		Timeout = 20

		var SyncOptionsOverrides SyncOptionsOverrides
		SyncOptionsOverrides.BgpStart = false
		SyncOptionsOverrides.Dryrun = false
		SyncOptionsOverrides.Timeout = Timeout

		t.Core, _ = CreateCore(t.g)

		if err = t.ExecSync(&SyncOptionsOverrides); err != nil {
			t.g.Log.Error(fmt.Sprintf("%s error executing objects syncing, err:'%s'",
				utils.GetGPid(), err))
			return err
		}
	}

	if err != nil {
		t.g.Log.Error(fmt.Sprintf("task failed, err:'%s'", err))
		return err
	}

	t.g.Log.Debug(fmt.Sprintf("%s child: finished task '%s' in '%s'",
		utils.GetGPid(), t.job.Id, time.Since(t0)))

	gspt.SetProcTitle(fmt.Sprintf(fmt.Sprintf("%s: waiting for jobs", t.CmdId)))

	return nil
}

// Executing a task as a process
func (t *Task) ExecProcess() error {
	var err error

	t0 := time.Now()

	t.g.Log.Debug(fmt.Sprintf("[%d] master: set request '%s' for execution job:'%s', parameters:'%s'",
		utils.GetGID(), t.job.Id, t.job.Type, t.job.Parameters))

	// Detecting exec binary by pid
	pid := os.Getpid()
	symlink := fmt.Sprintf("/proc/%d/exe", os.Getpid())
	var bin string
	if bin, err = os.Readlink(symlink); err != nil {
		t.g.Log.Error(fmt.Sprintf("%s exe binary for pid:'%d' status could not be detected, err:'%s'",
			utils.GetGPid(), pid, err))
		return err
	}

	t.g.Log.Debug(fmt.Sprintf("%s proc symlink:'%s' pointed to exe:'%s'",
		utils.GetGPid(), symlink, bin))

	// Running a process for task?
	cmd := exec.Command(bin, "server", "exec",
		fmt.Sprintf("{\"type\":\"%s\", \"parameters\":\"%s\"}",
			t.job.Type, t.job.Parameters), "--debug")

	cmd.Stdin = os.Stdin
	var out bytes.Buffer
	cmd.Stdout = &out
	cmd.Stderr = os.Stderr

	if err = cmd.Run(); err != nil {
		errstr := fmt.Sprintf("error running command, err:'%s'", err)
		return errors.New(errstr)
	}

	fmt.Printf("%s master: stdout:\n'\n%s\n'\n",
		utils.GetGPid(), out.String())

	t.g.Log.Debug(fmt.Sprintf("%s master: finished task '%s' in '%s'",
		utils.GetGPid(), t.job.Id, time.Since(t0)))

	return nil
}

func (t *Task) CheckIntervals(intervals string) bool {
	t0 := time.Now()

	t.g.Log.Debug(fmt.Sprintf("%s checking intervals:'%s' vs '%d'",
		utils.GetGPid(), intervals, t0.Minute()))

	var check bool

	check = false
	if len(intervals) == 0 {
		return true
	}

	tags := strings.Split(intervals, ",")
	for _, r := range tags {
		borders := strings.Split(r, "-")
		if len(borders) > 1 {
			var b0 int64
			var b1 int64
			var err error

			if b0, err = strconv.ParseInt(borders[0], 10, 64); err != nil {
				continue
			}
			if b1, err = strconv.ParseInt(borders[1], 10, 64); err != nil {
				continue
			}
			if t0.Minute() >= int(b0) && t0.Minute() <= int(b1) {
				t.g.Log.Debug(fmt.Sprintf("%s processing intervals:['%d', '%d'] vs '%d': OK",
					utils.GetGPid(), b0, b1, t0.Minute()))
				return true
			}
		}
	}

	return check
}

func (t *Task) ExecPlaceholder() error {

	var err error
	t0 := time.Now()
	id := "(placeholder)"

	gspt.SetProcTitle(fmt.Sprintf(fmt.Sprintf("%s: network objects sync", t.CmdId)))

	t.g.Log.Debug(fmt.Sprintf("%s %s: starting task",
		utils.GetGPid(), id))

	if t.Core == nil {
		err = errors.New("core is not initialized")
		t.g.Log.Error(fmt.Sprintf("error executing, err:'%s'", err))
		return err
	}

	if _, err = t.Core.sourceGetPaths(); err != nil {
		t.g.Log.Error(fmt.Sprintf("%s %s error getting object sources, err:'%s'",
			utils.GetGPid(), id, err))
		return err
	}

	// Signalling to event processing that
	// we need full check and resync (if needed)
	t.Core.notifies <- Notify{path: nil}

	t.g.Log.Debug(fmt.Sprintf("%s %s finished in '%s'",
		utils.GetGPid(), id, time.Since(t0)))

	gspt.SetProcTitle(fmt.Sprintf(fmt.Sprintf("%s: waiting for jobs", t.CmdId)))

	return err
}

// bgp: starting session and watching for
// some events. task function is called from
// (1) main process (2) command line
func (t *Task) ExecBgpStart(BgpOptionsOverrides *BgpOptionsOverrides) error {
	var err error
	t0 := time.Now()
	id := "(bgp)"

	options := ""
	t.g.Log.Debug(fmt.Sprintf("%s %s [task] started, options:'%s",
		utils.GetGPid(), id, options))

	if BgpOptionsOverrides.Timeout > 0 {
		// starting notification routing to process
		// updates from bgp side
		go t.Core.bgpNofifyCollector()
	}

	if err = t.GenericBgpStart(BgpOptionsOverrides); err != nil {
		t.g.Log.Error(fmt.Sprintf("error bgp session start, err:'%s'", err))
		return err
	}

	t.g.Log.Debug(fmt.Sprintf("%s %s [task] finished in '%s'",
		utils.GetGPid(), id, time.Since(t0)))

	return err
}

func (t *Task) GenericBgpStart(OptionsOverrides *BgpOptionsOverrides) error {

	var err error

	id := "(bgp) (thread)"

	safis := t.Core.getSafis()

	var wg sync.WaitGroup
	wg.Add(len(safis))

	for safi, p := range safis {
		go func(safi string) {
			var Options BgpOptionsOverrides
			Options.Timeout = OptionsOverrides.Timeout
			Options.Safi = safi

			t.g.Log.Debug(fmt.Sprintf("%s %s config starting bgp SAFI:'%s' peers:'%s'",
				utils.GetGPid(), id, safi, strings.Join(p, ",")))

			if err = t.Core.bgpStartSession(&Options); err != nil {
				t.g.Log.Error(fmt.Sprintf("error bgp session start, err:'%s'", err))
				return
			}
			wg.Done()
		}(safi)
	}

	if OptionsOverrides.Timeout > 0 {
		wg.Wait()
	}

	return err
}

// Getting status of some objects: (1) rib (2) network
func (t *Task) ExecStatus(StatusOptionsOverrides *StatusOptionsOverrides) error {
	var err error
	t0 := time.Now()
	id := "(status)"

	options := ""
	t.g.Log.Debug(fmt.Sprintf("%s %s [task] started, options:'%s",
		utils.GetGPid(), id, options))

	var paths []Path
	if paths, err = t.Core.bgpStatus(StatusOptionsOverrides); err != nil {
		t.g.Log.Error(fmt.Sprintf("error status, err:'%s'", err))
		return err
	}

	t.g.Log.Debug(fmt.Sprintf("%s %s [task] paths received count:'%d'",
		utils.GetGPid(), id, len(paths)))

	var objects TObjects
	if objects, err = t.Core.networkStatus(StatusOptionsOverrides); err != nil {
		t.g.Log.Error(fmt.Sprintf("error status, err:'%s'", err))
		return err
	}
	t.g.Log.Debug(fmt.Sprintf("%s %s [task] objects received %s",
		utils.GetGPid(), id, objects.AsString()))

	t.g.Log.Debug(fmt.Sprintf("%s %s [task] finished in '%s'",
		utils.GetGPid(), id, time.Since(t0)))

	return err
}

// Task to sync bgp session information and network
func (t *Task) ExecSync(SyncOptionsOverrides *SyncOptionsOverrides) error {
	var err error
	t0 := time.Now()
	id := "(sync)"

	gspt.SetProcTitle(fmt.Sprintf(fmt.Sprintf("%s: network objects sync", t.CmdId)))

	options := ""
	t.g.Log.Debug(fmt.Sprintf("%s %s [task] started, options:'%s",
		utils.GetGPid(), id, options))

	var BgpOptionsOverrides BgpOptionsOverrides
	BgpOptionsOverrides.Timeout = SyncOptionsOverrides.Timeout

	if t.g.Opts.Runtime.Source == config.SOURCE_BGP {
		if SyncOptionsOverrides.BgpStart {
			if err = t.GenericBgpStart(&BgpOptionsOverrides); err != nil {
				t.g.Log.Error(fmt.Sprintf("error bgp session start, err:'%s'", err))
				return err
			}
		}
	}

	// As bgp session ended or already established we got []Path
	if err = t.Core.networkSync(SyncOptionsOverrides); err != nil {
		t.g.Log.Error(fmt.Sprintf("error network sync, err:'%s'", err))
		return err
	}

	t.g.Log.Debug(fmt.Sprintf("%s %s [task] finished in '%s'",
		utils.GetGPid(), id, time.Since(t0)))

	gspt.SetProcTitle(fmt.Sprintf(fmt.Sprintf("%s: waiting for jobs", t.CmdId)))

	return err
}

// Task to flush network configuration
func (t *Task) ExecFlush(SyncOptionsOverrides *SyncOptionsOverrides) error {
	var err error
	t0 := time.Now()
	id := "(flush)"

	options := ""
	t.g.Log.Debug(fmt.Sprintf("%s %s [task] started, options:'%s",
		utils.GetGPid(), id, options))

	// Detecting current network configuration for tunnels/transits
	if err = t.Core.networkFlush(SyncOptionsOverrides); err != nil {
		t.g.Log.Error(fmt.Sprintf("error network flush, err:'%s'", err))
		return err
	}

	t.g.Log.Debug(fmt.Sprintf("%s %s [task] finished in '%s'",
		utils.GetGPid(), id, time.Since(t0)))

	return err
}

// Task to logrotate via client unix call
func (t *Task) Logrotate(LogrotateOptions *config.LogrotateOptions) error {
	var err error
	t0 := time.Now()
	id := "(logrotate)"

	options := ""
	t.g.Log.Debug(fmt.Sprintf("%s %s [task] started, options:'%s",
		utils.GetGPid(), id, options))

	if t.Core == nil {
		err = errors.New("core is not initialized")
		t.g.Log.Error(fmt.Sprintf("error executing, err:'%s'", err))
		return err
	}

	// Detecting current network configuration for tunnels/transits
	if err = t.Core.serverLogrotate(LogrotateOptions); err != nil {
		t.g.Log.Error(fmt.Sprintf("error server logrotate, err:'%s'", err))
		return err
	}

	t.g.Log.Debug(fmt.Sprintf("%s %s [task] finished in '%s'",
		utils.GetGPid(), id, time.Since(t0)))

	return err
}

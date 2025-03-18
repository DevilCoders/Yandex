package core

import (
	"errors"
	"fmt"
	"io/ioutil"
	"math/rand"
	"os"
	"os/exec"
	"os/signal"
	"strconv"
	"sync"
	"syscall"
	"time"

	"github.com/robfig/cron"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/utils"
)

type CoreOverrides struct {
	Detach bool `json:"detach"`
}

func (c *CoreOverrides) AsString() string {
	return fmt.Sprintf("detach:'%t'", c.Detach)
}

func (c *Core) StartCore(Overrides *CoreOverrides) error {

	if Overrides.Detach {
		var err error
		if os.Getppid() != 1 {
			// Detecting if process running already
			var run bool
			var pid int

			if run, pid, _, err = c.IfProcessRun(); err == nil && run {
				c.g.Log.Error(fmt.Sprintf("%s process already run on pid:'%d'",
					utils.GetGPid(), pid))
				return err
			}
		}

		if err = c.Daemon(); err != nil {
			c.g.Log.Error(fmt.Sprintf("error detaching process, err:'%s'", err))
			return err
		}
	}

	c.g.Log.Debug(fmt.Sprintf("%s starting core, overrides:'%s'",
		utils.GetGPid(), Overrides.AsString()))

	var waitGroup sync.WaitGroup
	// 1 - eventloop goroutine (listener wait)
	// 2 - cronloop goroutine (signal wait)
	// 3 - apiloop goroutine (signal wait)
	count := 1
	waitGroup.Add(count)

	api, _ := CreateApi(c.g)
	api.core = c
	c.httpapi = api

	// Integration after all starts
	c.CoreMonitor(c.g.Monitor)

	d := &Cronloop{g: c.g, core: c}

	// Periodically run tasks
	go d.Cronloop(&waitGroup)

	// API methods: unix socket and https
	// (for remote calls)
	go api.Apiloop(&waitGroup, API_UNIXSOCKET)
	//go api.Apiloop(&waitGroup, API_HTTPS)

	// Starting bgp session if enabled
	if c.g.Opts.Runtime.Source == config.SOURCE_BGP {
		bgp := c.g.Opts.Bgp
		if bgp.Enabled {
			go c.ProcessBgpStart()
		}

		// starting notification routing to process
		// updates from bgp side
		go c.bgpNofifyCollector()
	}

	c.g.Monitor.SetAsServerSide()

	waitGroup.Wait()

	return nil
}

func (c *Core) ProcessBgpStart() error {
	var task *Task
	var err error
	if task, err = CreateTask(c.g, TASK_NULL); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error creating bgp start task, err:'%s'",
			utils.GetGPid(), err))
		return err
	}
	task.Core = c

	var BgpOptionsOverrides BgpOptionsOverrides
	BgpOptionsOverrides.Timeout = BGPOPTIONS_STARTFOREVER

	if err = task.ExecBgpStart(&BgpOptionsOverrides); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error executing bgp start task, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	return err
}

// Non systemd process mode
func (c *Core) Daemon() error {

	// short delay to avoid race condition between os.StartProcess and os.Exit
	// can be omitted if the work done above amounts to a sufficient delay
	//time.Sleep(1 * time.Second)

	if os.Getppid() != 1 {
		// I am the parent, spawn child to run as daemon
		binary, err := exec.LookPath(os.Args[0])
		if err != nil {
			c.g.Log.Error(fmt.Sprintf("Failed to lookup binary, err:'%s'", err))
			return err
		}
		_, err = os.StartProcess(binary, os.Args, &os.ProcAttr{Dir: "", Env: nil,
			Files: []*os.File{os.Stdin, os.Stdout, os.Stderr}, Sys: nil})
		if err != nil {
			c.g.Log.Error(fmt.Sprintf("Failed to start process, err:'%s'", err))
			return err
		}

		program := c.g.Opts.Runtime.ProgramName
		c.g.Log.Debug(fmt.Sprintf("%s '%s' service is starting",
			utils.GetGPid(), program))

		os.Exit(0)
	} else {
		// I am the child, i.e. the daemon, start new session and detach from terminal
		_, err := syscall.Setsid()
		if err != nil {
			c.g.Log.Error(fmt.Sprintf("Failed to create new session: err:'%s'", err))
			return err
		}
		file, err := os.OpenFile("/dev/null", os.O_RDWR, 0)
		if err != nil {
			c.g.Log.Error(fmt.Sprintf("Failed to open /dev/null, err:'%s'", err))
			return err
		}
		syscall.Dup2(int(file.Fd()), int(os.Stdin.Fd()))
		syscall.Dup2(int(file.Fd()), int(os.Stdout.Fd()))
		syscall.Dup2(int(file.Fd()), int(os.Stderr.Fd()))
		file.Close()

		// writing pidfile
		pid := fmt.Sprintf("%d", os.Getpid())
		pidfile := c.g.Opts.M3.PidFile
		if err = ioutil.WriteFile(pidfile, []byte(pid), 0644); err != nil {
			c.g.Log.Error(fmt.Sprintf("%s error writing pidfile '%s', err:'%s'",
				utils.GetGPid(), pidfile, err))
			return err
		}
	}

	return nil
}

func (c *Core) IfProcessRun() (bool, int, *os.Process, error) {
	run := false
	var pid int64

	pidfile := c.g.Opts.M3.PidFile
	var content []byte
	var err error
	if content, err = ioutil.ReadFile(pidfile); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error reading pidfile '%s', err:'%s'",
			utils.GetGPid(), pidfile, err))
		return run, int(pid), nil, err
	}

	if len(content) == 0 {
		err = errors.New("empty pidfile")
		c.g.Log.Error(fmt.Sprintf("%s error reading pidfile '%s', err:'%s'",
			utils.GetGPid(), pidfile, err))
		return run, int(pid), nil, err
	}

	spid := fmt.Sprintf("%s", content)
	if pid, err = strconv.ParseInt(spid, 10, 64); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error converting pidfile '%s' content into int, err:'%s'",
			utils.GetGPid(), pidfile, err))
		return run, int(pid), nil, err
	}

	c.g.Log.Debug(fmt.Sprintf("%s detecting pidfile:'%s' pid:'%d'",
		utils.GetGPid(), pidfile, pid))

	var p *os.Process
	if p, err = os.FindProcess(int(pid)); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error detecting process, pid:'%d', err:'%s'",
			utils.GetGPid(), pid, err))
		return run, int(pid), nil, err
	}

	if err = p.Signal(syscall.SIGCONT); err != nil {
		err = errors.New("process not running")
		c.g.Log.Debug(fmt.Sprintf("%s detected pid:'%d' not running",
			utils.GetGPid(), pid))
		return run, int(pid), p, err
	}

	return true, int(pid), p, nil
}

func (c *Core) StopCore(Overrides *CoreOverrides) error {

	if Overrides.Detach {
		var err error
		var run bool
		var pid int
		var p *os.Process

		if run, pid, p, err = c.IfProcessRun(); err != nil {
			c.g.Log.Error(fmt.Sprintf("%s error detecting run process, pid:'%d', err:'%s'",
				utils.GetGPid(), pid, err))
			return err
		}

		if !run {
			err = errors.New("no running process detected")
			c.g.Log.Error(fmt.Sprintf("%s no detecting run process, pid:'%d', err:'%s'",
				utils.GetGPid(), pid, err))
			return err
		}

		c.g.Log.Debug(fmt.Sprintf("%s detected pid:'%d' running",
			utils.GetGPid(), pid))

		if err = p.Signal(syscall.SIGTERM); err != nil {
			c.g.Log.Error(fmt.Sprintf("%s error signalling pid:'%d', err:'%s'",
				utils.GetGPid(), pid, err))
			return err
		}
	}

	return nil
}

// Common function to handle goroutines without it own
// event loop (listen, bind or so on)
func WaitSignal(g *config.CmdGlobal, label string, wg *sync.WaitGroup) {
	gid := utils.GetGID()

	ch := make(chan os.Signal)
	//      signal.Notify(ch, syscall.SIGPWR)
	signal.Notify(ch, syscall.SIGINT)
	signal.Notify(ch, syscall.SIGQUIT)
	signal.Notify(ch, syscall.SIGTERM)

	sig := <-ch

	if sig == syscall.SIGQUIT {
		g.Log.Info(fmt.Sprintf("[%d] %s: received '%s signal', shutting down",
			gid, label, sig))
		return
	}
	g.Log.Info(fmt.Sprintf("[%d] %s: received '%s signal', exiting",
		gid, label, sig))

	/*        if listener != nil {
			g.Log.Info(fmt.Sprintf("[%d] %s -> also listener shutdown",
		                gid, label))
			if listener.IsActive() {
		                listener.Disconnect()
				wg.Done();
			}
	        } */

	return
}

type Cronloop struct {
	g     *config.CmdGlobal
	core  *Core
	mutex sync.Mutex
}

func (c *Cronloop) Placeholder() {
	if err := c.LunchTask("placeholder", "placeholder", false); err != nil {
		c.g.Log.Error(fmt.Sprintf("[%d] error creating task, err:'%s'",
			utils.GetGID(), err))
	}
	return
}

func (c *Cronloop) Logrotate() {
	if err := c.LunchTask("logrotate", "logrotate", false); err != nil {
		c.g.Log.Error(fmt.Sprintf("[%d] error creating task, err:'%s'",
			utils.GetGID(), err))
	}
	return
}

func (c *Cronloop) ObjectsSync() {
	if err := c.LunchTask("objects-sync", "objects-sync", true); err != nil {
		c.g.Log.Error(fmt.Sprintf("[%d] error creating task, err:'%s'",
			utils.GetGID(), err))
	}
	return
}

// Generic function to lunch task
func (c *Cronloop) LunchTask(job string, parameters string, fork bool) error {

	c.g.Log.Debug(fmt.Sprintf("[%d] lunch task, job:'%s', parameters:'%s', fork:'%t' waking up",
		utils.GetGID(), job, parameters, fork))

	var err error
	var task *Task
	if task, err = CreateTask(c.g, ""); err != nil {
		c.g.Log.Error(fmt.Sprintf("[%d] error creating task, err:'%s'",
			utils.GetGID(), err))
		return err
	}

	task.Core = c.core

	task.job.Type = job
	task.job.Parameters = parameters

	if fork {
		// Forking a process before task is executed
		err = task.ExecProcess()
		return err
	}

	if err = task.Exec(); err != nil {
		c.g.Log.Error(fmt.Sprintf("[%d] error executing task, err:'%s'",
			utils.GetGID(), err))
		return err
	}
	return nil
}

func (c *Cronloop) Cronloop(wg *sync.WaitGroup) {
	defer wg.Done()

	gid := utils.GetGID()
	rand.Seed(time.Now().UnixNano())
	ce := cron.New()

	var crons = []string{}

	// events intergration gc
	tag := "events-gc"
	crons = append(crons, tag)
	c.g.Log.Debug(fmt.Sprintf("%s tag:'%s' events gc d2 processing enabled: OK",
		utils.GetGPid(), tag))

	// syncing objects processing
	tags := map[string]func(){
		"placeholder": c.Placeholder,
	}

	if c.g.Opts.M3.Logrotate {
		tags["logrotate"] = c.Logrotate
	}

	for tag, _ := range tags {
		crons = append(crons, tag)
	}

	// If source is http we need sync it
	// periodically each one minute
	if c.g.Opts.Runtime.Source == config.SOURCE_HTTP {
		tag := "objects-sync"
		crons = append(crons, tag)
	}

	c.g.Monitor.OneTimeChecks()

	// monitoring integration, here we assume that all checks are
	// already defined before (with all properties needed)
	c.g.Monitor.PeriodicChecks(ce)

	for _, v := range crons {
		var err error
		var f cron.FuncJob

		hours := "*"
		minutes := "*"
		seconds := rand.Intn(59)

		switch v {
		case "events-gc":
			f = c.g.Events.CronGC
		case "placeholder":
			minutes = "*"
			f = tags[v]
		case "logrotate":
			minutes = "01"
			hours = "00"
			f = tags[v]
		case "objects-sync":
			f = c.ObjectsSync
		}

		// skipping some strange nodes without time interval
		// defined for any process
		if len(minutes) == 0 {
			continue
		}

		c.g.Log.Debug(fmt.Sprintf("%s cron:'%s', hours:'%s' minutes:'%s' seconds:'%d' scheduled: OK",
			utils.GetGPid(), v, hours, minutes, seconds))

		if err = ce.AddFunc(fmt.Sprintf("%d %s %s * * *", seconds, minutes, hours), f); err != nil {
			c.g.Log.Error(fmt.Sprintf("[%d] cronloop, error adding job, err:'%s'", gid, err))
			return
		}
	}

	// starting cron jobs
	ce.Start()

	WaitSignal(c.g, "cronloop", nil)
}

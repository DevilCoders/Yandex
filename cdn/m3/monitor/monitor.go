package monitor

// Monitor - is an implementation of program monitoring and
// its integration with juggler system

import (
	"encoding/json"
	"errors"
	"fmt"
	"math/rand"
	"net/http"
	"path"
	"sort"
	"strings"
	"sync"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/go-redis/redis"
	"github.com/robfig/cron"
	"github.com/sirupsen/logrus"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cdn/m3/utils"
)

const (
	MONITOR_SERVER = 210
	MONITOR_CLIENT = 211
)

// Maximum history checks in shared memory,
// before each insert we need to iterate thru all
// checks and delete outdated
const (
	MAX_HISTORY_CHECKS = 20
	MAX_TTL            = 2400
)

const (
	JUGGLER_OK       = 0
	JUGGLER_WARNING  = 1
	JUGGLER_CRITICAL = 2
)

var JUGGLER = map[int]string{JUGGLER_OK: "OK", JUGGLER_WARNING: "WARNING",
	JUGGLER_CRITICAL: "CRITICAL"}

// Monitor - is a core class to have all monitoring
// checks be implemented as a shared memory maps with
// some history structure. Some periodical process
// for each check runs corresponding linked to a check
// function/goroutine and fills the monitoring check
// state. It can handle upto N historically checks/metrics
type Monitor struct {

	// a map of shared maps, each sync.map has a time
	// key from now till the end of history
	checks map[string]sync.Map
	mutex  *sync.RWMutex

	// a map of last check pointers (keys from checks map)
	ptrs sync.Map

	// an array for checks configuration
	config map[string]CheckConfig

	// Integration variables from outside
	// of monitor: logs
	log *logrus.Logger

	// If not null, all client command for monitor
	// are bond to this pointer
	cmd *cobra.Command

	// Api handler integration
	gin *gin.Engine
	// Api unix socket
	socket string

	// Program and class (as base path from program)
	program string

	// Juggler has section to group all checks, e.g. d2, hx, ...
	section string

	// Two possible modes: server (so all the results push
	// are inserted into map), and client (just checks w/o push)
	mode int

	// client run exec some monitoring actions, e.g.
	// running journal compactions on detected failed
	// zones
	exec bool

	// GC mutex
	gcm sync.Mutex

	// Graphite Metrics Configuration, e.g. dns-data.v3
	prefix string

	// Redis instance to push data
	redis string

	// New way to pass parameters - just as variables
	SolomonMetrics SolomonMetrics

	// Graphite metrics
	GraphiteMetrics GraphiteMetrics

	// Juggler (raw events)
	JugglerMetrics JugglerMetrics

	command string
}

type JugglerMetrics struct {
	Host    string
	Service string
	Url     string
}

type GraphiteMetrics struct {
	Enabled bool
}

type SolomonMetrics struct {
	Enabled  bool
	Endpoint string
	Project  string
	Cluster  string
	Host     string
	Token    string

	Service  string
	Location string
	Role     string
}

// Metrics: could be used in graphite or
// solomon system (tagged)
type Value struct {
	Id    string            `json:"id"`
	Value float64           `json:"value"`
	Tags  map[string]string `json:"tags"`
}

func (v *Value) AsString() string {
	tags := ""
	if v.Tags != nil {
		for k, w := range v.Tags {
			t := fmt.Sprintf("'%s':'%s'", k, w)
			if len(tags) > 0 {
				tags = fmt.Sprintf("%s;%s", tags, t)
				continue
			}
			tags = t
		}
	}

	if len(tags) == 0 {
		return fmt.Sprintf("id:'%s', v:'%2.2f'", v.Id, v.Value)
	}

	return fmt.Sprintf("id:'%s' v:'%2.2f', tags:'%s'",
		v.Id, v.Value, tags)
}

type Metrics map[string]Value

// A check implements juggler check and some monitoring metrics
// used to send them into graphite collector?
type Check struct {
	ID        string    `json:"id"`
	Class     string    `json:"class"`
	Timestamp time.Time `json:"timestamp"`
	TTL       float64   `json:"ttl"`

	Code    int64  `json:"code"`
	Message string `json:"message"`

	Metrics *Metrics `json:"metrics,omitempty"`
}

// Function should be a cron-like job
type CheckFunction func()

type CheckConfig struct {
	ID string `json:"id"`
	F  CheckFunction
}

func (c *Check) AsString() string {
	t0 := time.Now()
	age := t0.Sub(c.Timestamp).Seconds()

	return fmt.Sprintf("check, id:'%s', timestamp:'%s', ts:'%d', ttl:'%2.2f, age:'%2.4f' seconds,  code:'%d', message:'%s''",
		c.ID, c.Timestamp, c.Timestamp.UnixNano(), c.TTL, age, c.Code, c.Message)
}

// Creating a monitor command and object
func CreateMonitor(program string, log *logrus.Logger, cmd *cobra.Command,
	socket string, prefix string, redis string, command string) *Monitor {
	var m Monitor

	m.log = log

	m.command = command

	m.socket = socket
	m.program = program

	// monrun/juggler class is a base of program
	m.section = path.Base(m.program)

	m.checks = make(map[string]sync.Map)
	m.mutex = &sync.RWMutex{}

	m.config = make(map[string]CheckConfig)
	m.prefix = prefix
	m.redis = redis

	// Adding monitor-type cobra commands as
	// defined in events/commands.go
	if cmd != nil {
		m.cmd = cmd
		monitorCmd := cmdMonitor{monitor: &m, command: command}
		m.cmd.AddCommand(monitorCmd.Command())
	}

	//m.log.Debug(fmt.Sprintf("%s created monitor core object", utils.GetGPid()))

	// This property is set as server started
	m.mode = MONITOR_CLIENT

	return &m
}

func (m *Monitor) GetLogLevel() logrus.Level {
	return m.log.Level
}

func (m *Monitor) SetLog(log *logrus.Logger) {
	m.log = log
}

func (m *Monitor) SetGraphitePreifx(prefix string) {
	m.prefix = prefix
}

func (m *Monitor) SetGraphiteEndpoint(redis string) {
	m.redis = redis
}

func (m *Monitor) SetAsServerSide() {
	m.mode = MONITOR_SERVER
}

func (m *Monitor) ModeAsString() string {
	modes := map[int]string{
		MONITOR_CLIENT: "monitor/client",
		MONITOR_SERVER: "monitor/server",
	}
	return modes[m.mode]
}

func (m *Monitor) GetMode() int {
	return m.mode
}

func (m *Monitor) GetExec() bool {
	return m.exec
}

func (m *Monitor) AsString() string {
	return fmt.Sprintf("monitor, checks id count:'%d'", len(m.checks))
}

func (m *Monitor) AddConfig(c CheckConfig) {
	m.config[c.ID] = c
}

// Executing a check monioring, could be run as system
// process or command line client call
func (m *Monitor) Exec(id string, exec bool) error {
	t0 := time.Now()

	m.log.Debug(fmt.Sprintf("%s started check id:'%s' as exec:'%t'",
		utils.GetGPid(), id, exec))

	if _, ok := m.config[id]; !ok {
		return errors.New(fmt.Sprintf("No monitoring check detected with id:'%s'", id))
	}
	if m.config[id].F == nil {
		return errors.New(fmt.Sprintf("Monitoring check is no set for id:'%s'", id))
	}

	m.exec = exec

	// Running checks from checks array
	m.config[id].F()

	m.log.Debug(fmt.Sprintf("%s finished check id:'%s' in '%s'",
		utils.GetGPid(), id, time.Since(t0)))

	return nil
}

func (m *Monitor) SendMetrics() {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	m.log.Debug(fmt.Sprintf("%s sending metrics to graphite",
		utils.GetGPid()))

	var keys []string
	m.ptrs.Range(func(k, v interface{}) bool {
		keys = append(keys, k.(string))
		return true
	})

	// a key for metrics map (some) uniq value
	// but actual id is in Value object denoted as Id
	metrics := make(map[string]Value)

	for _, id := range keys {
		if v, ok := m.ptrs.Load(id); ok {
			if _, ok := m.checks[id]; ok {
				s := m.checks[id]
				if w, ok := s.Load(v); ok {
					check := w.(Check)
					if check.Metrics == nil {
						continue
					}
					for k, f := range *check.Metrics {
						m.log.Debug(fmt.Sprintf("%s monitor id:'%s', k:'%d', metrics:%s -> %s",
							utils.GetGPid(), id, v, k, f.AsString()))
						metrics[k] = f
					}
				}
			}
		}
	}

	if m.SolomonMetrics.Enabled {
		m.SendSolomonMetrics(metrics)
	}

	if m.GraphiteMetrics.Enabled {
		// Connecting to redisdb via master
		var err error
		redisdb := redis.NewClient(&redis.Options{
			Addr:     m.redis,
			Password: "",
			DB:       0,
		})
		if _, err = redisdb.Ping().Result(); err != nil {
			m.log.Error(fmt.Sprintf("%s monitor graphite-sender redis connection failed, err:'%s'",
				utils.GetGPid(), err))
			return
		}

		pipe := redisdb.Pipeline()

		// Processing metrics in a pipe sending data to redis/graphite?
		for k, v := range metrics {
			key := fmt.Sprintf("%s.%s", m.prefix, k)

			m.log.Debug(fmt.Sprintf("%s monitor graphite-sender k:'%s' -> v'%2.2f'",
				utils.GetGPid(), key, v.Value))

			pipe.Set(key, v.Value, 0)
			pipe.Expire(key, 180*time.Second)
		}

		if _, err = pipe.Exec(); err != nil {
			m.log.Error(fmt.Sprintf("error writing data, err:%s", err))
		}

		redisdb.Close()
	}
}

func (m *Monitor) PeriodicChecks(cron *cron.Cron) error {
	var err error

	config := make(map[string]CheckConfig)
	for k, v := range m.config {
		config[k] = v
	}
	config["metrics-sender"] = CheckConfig{ID: "metrics-sender", F: m.SendMetrics}

	for k, v := range config {
		// one minute seconds
		time := rand.Intn(59)
		m.log.Debug(fmt.Sprintf("%s setting cron started time for id:'%s' at '%d' seconds",
			utils.GetGPid(), k, time))

		if err = cron.AddFunc(fmt.Sprintf("%d * * * * *", time), v.F); err != nil {
			m.log.Error(fmt.Sprintf("%s monitor, error adding check task, err:'%s'",
				utils.GetGPid(), err))
			return err
		}
	}

	return err
}

// Function to run all checks as an application start
func (m *Monitor) OneTimeChecks() error {
	for _, v := range m.config {
		v.F()
	}
	return nil
}

func (m *Monitor) PushCheck(c Check) {
	m.log.Debug(fmt.Sprintf("%s %s push check into shared map, check:%s",
		utils.GetGPid(), m.ModeAsString(), c.AsString()))

	if m.mode == MONITOR_SERVER {
		// checking if corresponing shared sync map
		// exists in a map
		m.mutex.Lock()

		ts := c.Timestamp.UnixNano()
		if _, ok := m.checks[c.ID]; !ok {
			m.log.Debug(fmt.Sprintf("%s monitor/server created shared map, id:'%s'",
				utils.GetGPid(), c.ID))

			var s sync.Map
			s.Store(ts, c)
			m.checks[c.ID] = s
		} else {
			m.log.Debug(fmt.Sprintf("%s monitor/server use shared map, id:'%s'",
				utils.GetGPid(), c.ID))
			s := m.checks[c.ID]
			s.Store(ts, c)
		}

		// Setting last check pointer
		m.ptrs.Store(c.ID, ts)

		m.mutex.Unlock()

		m.GarbageCollectorChecks()

	}

	if m.mode == MONITOR_CLIENT {
		m.JugglerShowChecker(c)
	}
}

// Simple function that returns last check, see alternative
// version GetLastHistoryCheck (plus history tendancy)
func (m *Monitor) GetLastCheck(id string) (Check, error) {
	var check Check

	if v, ok := m.ptrs.Load(id); ok {
		if _, ok := m.checks[id]; ok {
			s := m.checks[id]
			if w, ok := s.Load(v); ok {
				check = w.(Check)

				m.log.Debug(fmt.Sprintf("%s monitor/server last-check id:'%s', k:'%d', check:%s",
					utils.GetGPid(), id, v, check.AsString()))

				return check, nil
			}
		}
	}

	return check, errors.New(fmt.Sprintf("No check found with id:'%s'", id))
}

func (m *Monitor) StatsAsString(stats map[int64]int) string {
	out := ""
	count := 0
	for k, _ := range JUGGLER {
		count += stats[int64(k)]
	}
	if count == 0 {
		return fmt.Sprintf("N/A")
	}
	for k, v := range JUGGLER {
		if stats[int64(k)] == 0 {
			continue
		}
		p := fmt.Sprintf("%s:%2.2f%%", v,
			100*float64(stats[int64(k)])/float64(count))
		if len(out) == 0 {
			out = p
			continue
		}
		out = fmt.Sprintf("%s %s", out, p)
	}
	return fmt.Sprintf("history: count:%d checks:'%s'", count, out)
}

// History enabled function with last check also history
// array of data is returned
func (m *Monitor) GetLastHistoryCheck(id string) (Check, error) {
	var err error
	var check Check

	// Statisctics for each monitoring types: OK, WARN, CRIT
	stats := make(map[int64]int)
	for k, _ := range JUGGLER {
		stats[int64(k)] = 0
	}

	// We need iterate from last timestamp upto some
	// past data, calculating, counts for codes: OK:100%,
	// WARN:0%, CRIT:0%
	m.mutex.Lock()

	if s, ok := m.checks[id]; ok {
		var keys []int64
		s.Range(func(k, v interface{}) bool {
			keys = append(keys, k.(int64))
			return true
		})

		// Sorting history checks to process first the most current
		sort.Slice(keys, func(i, j int) bool { return keys[i] > keys[j] })

		// TODO: could be analizied some flaps/OK/WARN/CRIT
		max := 5
		for i, k := range keys {
			if v, ok := s.Load(k); ok {
				a := v.(Check)
				stats[a.Code] = stats[a.Code] + 1

				if i < max {
					m.log.Debug(fmt.Sprintf("%s monitor/history [%d]/[%d]: key: %d, check:'%s', stats:'%d'",
						utils.GetGPid(), i, len(keys), k, a.AsString(), stats[a.Code]))
				}
			}
		}
	}
	m.mutex.Unlock()

	if check, err = m.GetLastCheck(id); err != nil {
		return check, err
	}

	m.log.Debug(fmt.Sprintf("%s monitor/history id:'%s' stats:'%s' debug:'%+v'",
		utils.GetGPid(), id, m.StatsAsString(stats), stats))

	check.Message = fmt.Sprintf("%s %s", check.Message, m.StatsAsString(stats))

	return check, nil
}

func (m *Monitor) GarbageCollectorChecks() {

	m.gcm.Lock()
	defer m.gcm.Unlock()

	var outdated []int64
	t0 := time.Now()

	count := 0
	for id, s := range m.checks {

		// TODO: Checking for last-check method
		m.GetLastCheck(id)
		m.GetLastHistoryCheck(id)

		m.mutex.Lock()
		s.Range(func(k, v interface{}) bool {
			count++
			a := v.(Check)
			age := t0.Sub(a.Timestamp).Seconds()

			// if MAX_TTL around 20 minutes, so we have 20 values for
			// last checks performed, some statistics/flaps could be
			// detected
			if age > MAX_TTL {
				m.log.Debug(fmt.Sprintf("%s monitor/server gc id:'%s', k:'%d', outdated: age:'%2.6f' seconds",
					utils.GetGPid(), id, k, age))
				outdated = append(outdated, k.(int64))
			}
			return true
		})

		// We have outdated keys to be deleted
		for i, k := range outdated {
			if v, ok := s.Load(k); ok {
				a := v.(Check)
				m.log.Debug(fmt.Sprintf("%s monitor/server [%d]/[%d]/[%d]: gc purge key: %d, check:'%s'",
					utils.GetGPid(), i+1, len(outdated), count, k, a.AsString()))
				s.Delete(k)
			}
		}
		m.mutex.Unlock()

		m.log.Debug(fmt.Sprintf("%s monitor/server gc items purged:[%d]/[%d]",
			utils.GetGPid(), len(outdated), count))
	}
}

func (m *Monitor) JugglerConfig() {
	count := 0
	for _, k := range m.config {
		count++
		tags := strings.Split(k.ID, fmt.Sprintf("%s-", m.section))
		id := k.ID
		if len(tags) > 1 {
			id = tags[1]
		}

		// We have some new rules for juggler checks
		if utils.StringInSlice(m.program, []string{"/usr/bin/cc-core"}) {
			id = k.ID
		}

		// a list of runttime checks with run and high-speed checks
		runtime := []string{"d2-dns-traffic"}

		if utils.StringInSlice(k.ID, runtime) {
			fmt.Printf("# [%d]/[%d], id:'%s'\n", count, len(m.config), id)
			fmt.Printf("[%s]\n", id)
			fmt.Printf("execution_interval=%d\n", 5)
			fmt.Printf("execution_timeout=%d\n", 10)
			fmt.Printf("command=%s %s run --juggler %s\n", m.program, m.command, k.ID)
			fmt.Printf("type=%s\n\n", m.section)
			continue
		}

		fmt.Printf("# [%d]/[%d], id:'%s'\n", count, len(m.config), id)
		fmt.Printf("[%s]\n", id)
		fmt.Printf("execution_interval=%d\n", 60)
		fmt.Printf("execution_timeout=%d\n", 20)
		fmt.Printf("command=%s %s list --juggler %s\n", m.program, m.command, k.ID)
		fmt.Printf("type=%s\n\n", m.section)
	}
}

func (m *Monitor) JugglerStubMessage(code int, id string, message string) {
	fmt.Printf("%d;%s: %s: %s\n", code, id, message, JUGGLER[code])
}

func (m *Monitor) JugglerShowChecker(c Check) {
	t0 := time.Now()

	age := t0.Sub(c.Timestamp).Seconds()
	tags := strings.Split(c.ID, fmt.Sprintf("%s-", m.section))
	id := c.ID
	if len(tags) > 1 {
		id = tags[1]
	}

	if len(id) > 0 {
		fmt.Printf("%d;%s: %s: %s, age:'%2.2f' seconds\n", c.Code, id,
			c.Message, JUGGLER[int(c.Code)], age)
	}
}

type Sensor struct {
	Labels map[string]string `json:"labels"`
	Ts     int64             `json:"ts"`
	Value  float64           `json:"value"`
}

type CommonLabels struct {
	Cluster string `json:"cluster"`
	Host    string `json:"host"`
	Project string `json:"project"`
	Service string `json:"service"`
}

type SolomonEnvelop struct {
	CommonLabels CommonLabels `json:"commonLabels"`
	Sensors      []Sensor     `json:"sensors"`
}

// Function to push data as a map of data
func (m *Monitor) SendSolomonMetrics(metrics map[string]Value) error {
	// converting map into array and use SendSolomonsMetrics
	var sensors []Value
	for _, s := range metrics {
		sensors = append(sensors, s)
	}
	return m.SendSolomonsMetrics(sensors, true)
}

// Function to push data as an array of data
func (m *Monitor) SendSolomonsMetrics(metrics []Value, role bool) error {
	var err error

	t0 := time.Now()
	m.log.Debug(fmt.Sprintf("%s monitor/server solomon metrics send, metrics count:'%d'",
		utils.GetGPid(), len(metrics)))

	var SolomonEnvelop SolomonEnvelop
	SolomonEnvelop.CommonLabels.Cluster = m.SolomonMetrics.Cluster
	SolomonEnvelop.CommonLabels.Host = m.SolomonMetrics.Host
	SolomonEnvelop.CommonLabels.Project = m.SolomonMetrics.Project

	//	if strings.HasPrefix(m.SolomonMetrics.Host, "alpha-") {
	//		m.SolomonMetrics.Service = "unknown"
	//	}

	SolomonEnvelop.CommonLabels.Service = m.SolomonMetrics.Service
	SolomonEnvelop.CommonLabels.Service =
		strings.Replace(SolomonEnvelop.CommonLabels.Service, "+", "-", -1)

	for _, v := range metrics {
		var Sensor Sensor
		Sensor.Ts = time.Now().Unix()
		Sensor.Value = v.Value
		Sensor.Labels = make(map[string]string)
		Sensor.Labels["geo"] = m.SolomonMetrics.Location
		Sensor.Labels["sensor"] = v.Id
		if role {
			Sensor.Labels["role"] = m.SolomonMetrics.Role
		}

		// Adding tags from map "tags"
		if v.Tags != nil {
			for l, w := range v.Tags {
				Sensor.Labels[l] = w
			}
		}

		SolomonEnvelop.Sensors = append(SolomonEnvelop.Sensors, Sensor)
	}

	var content []byte
	if content, err = json.Marshal(SolomonEnvelop); err != nil {
		m.log.Error(fmt.Sprintf("%s monitor/server solomon json marshalling error, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	//m.log.Debug(fmt.Sprintf("%s monitor/server data content generated as '%s'",
	//	utils.GetGPid(), string(content)))

	// http://solomon.yandex.net/api/v2
	url := fmt.Sprintf("%s/push?project=%s&cluster=%s&service=%s",
		m.SolomonMetrics.Endpoint, m.SolomonMetrics.Project,
		m.SolomonMetrics.Cluster, SolomonEnvelop.CommonLabels.Service)

	m.log.Debug(fmt.Sprintf("%s monitor/server solomon url api: '%s' token len:'%d'",
		utils.GetGPid(), url, len(m.SolomonMetrics.Token)))

	var result []byte
	var code int
	if result, err, code = utils.HttpRequest(http.MethodPost, url,
		m.SolomonMetrics.Token, content, nil, ""); err != nil {
		m.log.Error(fmt.Sprintf("%s monitor/server http request error, code:'%d', err:'%s'",
			utils.GetGPid(), code, err))
		return err
	}

	m.log.Debug(fmt.Sprintf("%s monitor/server post returned with '%s'",
		utils.GetGPid(), string(result)))

	m.log.Debug(fmt.Sprintf("%s monitor/server solomon sending finished in '%s'",
		utils.GetGPid(), time.Since(t0)))

	return nil
}

// juggler client provides local tcp port to push raw juggler
// items as an array of events
// envelope: {"source": "hx", "events": []}
// events array: {"host": "stw2-rp1.yndx.net", "service": "rdr-metrics-hx-alpha", "status": "OK", "description": "Interfaces: ['vlan991']", "tags": ["rdr", "hx-link-load"]},

type TItemJuggler struct {
	Id     string
	Status int64
}

type TRawJugglerEvent struct {
	Host        string   `json:"host"`
	Service     string   `json:"service"`
	Status      string   `json:"status"`
	Description string   `json:"description"`
	Tags        []string `json:"tags"`
}

func (j *TRawJugglerEvent) AsString() string {
	return fmt.Sprintf("host:'%s', service:'%s', status:'%s', description:'%s' tags:'%s'",
		j.Host, j.Service, j.Status, j.Description, strings.Join(j.Tags, ", "))
}

type TRawJugglerEnvelope struct {
	Source string             `json:"source"`
	Events []TRawJugglerEvent `json:"events"`
}

// Pushing events in raw juggler envelope
func (m *Monitor) PushEvent(r *TRawJugglerEnvelope, service string,
	desc string, code int, tags []string) {

	var j TRawJugglerEvent

	var statuses = map[int]string{0: "OK", 1: "WARN", 2: "CRIT"}

	j.Host = m.JugglerMetrics.Host

	j.Service = strings.ToLower(service)
	j.Status = statuses[code]
	j.Tags = append(j.Tags, m.JugglerMetrics.Host)
	for _, t := range tags {
		j.Tags = append(j.Tags, t)
	}

	j.Description = desc

	// checking if Events already contains an event with the same
	// Service id, we need merge it
	for i, e := range r.Events {
		if e.Service == j.Service {
			// Need update
			m.log.Debug(fmt.Sprintf("events aggregated %s desc:'%s' -> desc:'%s+%s'",
				j.Service, e.Description, e.Description, j.Description))

			r.Events[i].Description = fmt.Sprintf("%s %s",
				e.Description, j.Description)

			if j.Status == "CRIT" || e.Status == "CRIT" {
				r.Events[i].Status = "CRIT"
			}
			if j.Status == "WARN" || e.Status != "CRIT" {
				r.Events[i].Status = "WARN"
			}
			return
		}
	}

	r.Events = append(r.Events, j)
	m.log.Debug(fmt.Sprintf("%s %s", j.Status, desc))
}

func (m *Monitor) SendRawJuggler(rje TRawJugglerEnvelope) error {
	var err error
	var json string
	rje.Source = m.JugglerMetrics.Service

	url := m.JugglerMetrics.Url

	// Skipping if url is not set
	if len(url) == 0 {
		return err
	}

	if json, err = utils.ConvMarshal(rje, "JSON+RAW"); err != nil {
		m.log.Debug(fmt.Sprintf("error convering raw events json, err:'%s'", err))
		return err
	}

	m.log.Debug(fmt.Sprintf("%s events json formed as: '%s'",
		utils.GetGPid(), json))

	m.log.Debug(fmt.Sprintf("%s sending events to juggler url:'%s'",
		utils.GetGPid(), url))

	var response []byte
	var code int

	if response, err, code = utils.HttpRequest(http.MethodPost, url, "",
		[]byte(json), nil, ""); err != nil {
		m.log.Debug(fmt.Sprintf("error sending data, code:'%d' err:'%s'", code, err))
		return nil
	}

	m.log.Debug(fmt.Sprintf("%s responsed as: '%s'",
		utils.GetGPid(), response))

	return nil
}

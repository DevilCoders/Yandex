package events

// Events - is an implementation of program events, it could
// be created/deteled in any goroutie of program (as it is created
// via sync.Map) and be listed/monitored via methods it provides

import (
	"errors"
	"fmt"
	"path"
	"sync"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/sirupsen/logrus"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cdn/m3/utils"
)

const (
	MAX_EVENTS = 100
	EVENTS_TTL = 300
)

const (
	CLASS_EVENT_ERROR = "error"
)

// Events could be (1) errors (2) notifications
type Events struct {
	// shared sync map to store events
	// in application
	events sync.Map

	// Counting number of events
	ccm   sync.Mutex
	count int

	// GC mutex
	gcm sync.Mutex

	// Integration variables from outside
	// of events: logs
	log *logrus.Logger

	// If not null, all client command for events
	// are bond to this pointer
	cmd *cobra.Command

	// Api handler integration
	gin *gin.Engine
	// Api unix socket
	socket string

	// Program and class (as base path from program)
	program string
	class   string
}

type Event struct {
	Class     string    `json:"class"`
	Timestamp time.Time `json:"timestamp"`
	ID        string    `json:"string"`
	TTL       float64   `json:"ttl"`
}

type ErrorEvent struct {
	Event

	Code    int64  `json:"code"`
	Message string `json:"message"`
}

func (e *ErrorEvent) AsString() string {
	return fmt.Sprintf("event, id:'%s', timestamp:'%s', ttl:'%2.2f, code:'%d', message:'%s''",
		e.ID, e.Timestamp, e.TTL, e.Code, e.Message)
}

func CreateEvents(program string, log *logrus.Logger, cmd *cobra.Command, socket string) *Events {
	var e Events
	e.count = 0
	e.log = log

	e.socket = socket
	e.program = program
	// monrun/juggler class is a base of program
	e.class = path.Base(e.program)

	// Adding events-type cobra commands as
	// defined in events/commands.go
	if cmd != nil {
		e.cmd = cmd
		eventsCmd := cmdEvents{events: &e}
		e.cmd.AddCommand(eventsCmd.Command())
	}

	return &e
}

func (e *Events) PushError(code int64, message string) error {

	if e.count > MAX_EVENTS {
		return errors.New(fmt.Sprintf("Out of memory for events, max events count:'%d'", MAX_EVENTS))
	}

	var ee ErrorEvent
	ee.ID, _ = utils.Id()
	ee.Timestamp = time.Now()
	ee.TTL = EVENTS_TTL
	ee.Class = CLASS_EVENT_ERROR
	ee.Code = code
	ee.Message = message

	e.events.Store(ee.ID, ee)

	e.ccm.Lock()
	e.count++
	e.ccm.Unlock()

	e.log.Debug(fmt.Sprintf("%s events: queue: %s, pusing error event: %s",
		utils.GetGPid(), e.AsString(), ee.AsString()))

	return nil
}

func (e *Events) AsString() string {
	return fmt.Sprintf("[%d]/[%d] events: event queue", e.count, MAX_EVENTS)
}

// GC is garbaging collection of events: deleting all events with
// Timestamp + TTL more than current time
func (e *Events) GC(class string) error {

	e.log.Debug(fmt.Sprintf("%s events: gc, class:'%s'",
		utils.GetGPid(), class))

	e.gcm.Lock()
	defer e.gcm.Unlock()

	var outdated []string
	t0 := time.Now()

	e.events.Range(func(k, v interface{}) bool {
		a := v.(ErrorEvent)
		age := t0.Sub(a.Timestamp).Seconds()
		if age > a.TTL || class == "force" {
			outdated = append(outdated, fmt.Sprintf("%s", k))
		}
		return true
	})

	// We have outdated keys to be deleted
	for i, k := range outdated {
		if v, ok := e.events.Load(k); ok {
			a := v.(ErrorEvent)
			e.log.Debug(fmt.Sprintf("%s [%d]/[%d]/[%d] events: gc purged: %s",
				utils.GetGPid(), i+1, len(outdated), e.count, a.AsString()))

			e.events.Delete(k)
			e.ccm.Lock()
			e.count--
			e.ccm.Unlock()
		}
	}

	e.log.Debug(fmt.Sprintf("%s events: gc: items purged:[%d]/[%d]",
		utils.GetGPid(), len(outdated), e.count))

	return nil
}

func (e *Events) CronGC() {
	class := "normal"
	e.log.Debug(fmt.Sprintf("%s events/cron: gc, class:'%s'",
		utils.GetGPid(), class))
	e.GC(class)
}

// GetEvents retrives a list of events of specfied class
func (e *Events) GetErrorEvents(class string) ([]ErrorEvent, error) {
	var eve []ErrorEvent

	e.log.Debug(fmt.Sprintf("%s events: got getevents request class:'%s', queuelen:'%d'",
		utils.GetGPid(), class, e.count))
	t0 := time.Now()

	e.events.Range(func(k, v interface{}) bool {
		a := v.(ErrorEvent)
		eve = append(eve, a)
		return true
	})

	e.log.Debug(fmt.Sprintf("%s got events, count:'%d'",
		utils.GetGPid(), len(eve)))

	e.log.Debug(fmt.Sprintf("%s events command fininshed in '%s'",
		utils.GetGPid(), -t0.Sub(time.Now())))
	return eve, nil
}

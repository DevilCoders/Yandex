package events

// Monitor implements juggler-based events handling for
// monrun (locally generated monitoring) and for raw
// messages

import (
	"errors"
	"fmt"
	"time"

	"a.yandex-team.ru/cdn/m3/utils"
)

const (
	JUGGLER_OK       = 0
	JUGGLER_WARNING  = 1
	JUGGLER_CRITICAL = 2
)

// Switches from command line
type MonitorSwitches struct {
	ShowGraphite          bool
	ShowJuggler           bool
	ShowStatus            bool
	GenerateJugglerConfig bool
}

type Monitor struct {
	events *Events
	sw     MonitorSwitches
	items  string
}

func (m *Monitor) ShowItem(class string, id string, text string, state int) {
	var err = map[int]string{0: "OK", 1: "WARNING", 2: "CRITICAL"}
	var errstr = fmt.Sprintf("%d;%s: %s: %s", state, id, text, err[state])

	if m.sw.ShowJuggler && !m.sw.GenerateJugglerConfig {
		fmt.Printf("%s\n", errstr)
	}

	if m.sw.GenerateJugglerConfig {
		fmt.Printf("[%s]\n", fmt.Sprintf("%s-%s", class, id))
		fmt.Printf("execution_interval=%d\n", 60)
		fmt.Printf("execution_timeout=%d\n", 20)
		fmt.Printf("command=%s events monitor --juggler %s\n", m.events.program, id)
		fmt.Printf("type=%s\n\n", m.events.class)
	}

	m.events.log.Debug(fmt.Sprintf("%s", errstr))
}

// Events monitoring handler, please note that all error here
// is actually a error message in juggler not in stdout
func (m *Monitor) Events() error {

	var err error
	var code int
	var ee []ErrorEvent

	// getting events of error class
	class := "error"
	id := fmt.Sprintf("%s-%s", m.events.class, "events")
	if ee, code, err = m.events.apiClientGetEvents(class); err != nil {
		state := JUGGLER_CRITICAL
		if code == 404 {
			state = JUGGLER_OK
			m.ShowItem(m.events.class, id, fmt.Sprintf("no error events detected, OK"), state)
			return nil
		}
		m.ShowItem(m.events.class, id, fmt.Sprintf("error getting events, err:'%s'", err), state)

		m.events.log.Debug(fmt.Sprintf("[%d] error getting events, err:'%s'",
			utils.GetGID(), err))
		return err
	}

	m.events.log.Debug(fmt.Sprintf("[%d] got:'%d' events for class:'%s'",
		utils.GetGID(), len(ee), class))

	if len(ee) > 0 {
		t0 := time.Now()
		var maxage float64
		var minage float64
		for i, v := range ee {
			age := t0.Sub(v.Timestamp).Seconds()
			if i == 0 {
				maxage = age
				minage = age
				continue
			}
			if age < minage {
				minage = age
			}
			if age > maxage {
				maxage = age
			}
		}

		err = errors.New(fmt.Sprintf("error events detected, count:'%d', ages:[min:%2.2f, max:%2.2f],  zero-message:'%s'",
			len(ee), minage, maxage, ee[0].Message))
		m.ShowItem(m.events.class, id, fmt.Sprintf("%s", err), JUGGLER_CRITICAL)
		return err
	}

	return nil
}

package calendarclient

import (
	"encoding/json"
	"fmt"
	"strconv"
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/shift"
	"a.yandex-team.ru/cloud/ps/gore/pkg/calendar"
	"a.yandex-team.ru/library/go/core/log"
)

func init() {
	service.ImportAdd("calendar")
}

type Config struct {
	Log  log.Logger
	Auth struct {
		Method string
		KWArgs map[string]string
	}
	BaseURL        string
	CreateEventURL string
	DeleteEventURL string
	GetEventsURL   string
	GetEventURL    string
	ShareEventURL  string
}

func DefaultConfig() *Config {
	return &Config{
		BaseURL:        "http://calendar-api.tools.yandex.net/internal/",
		CreateEventURL: "create-event.json",
		DeleteEventURL: "delete-event.json",
		GetEventsURL:   "get-events.json",
		GetEventURL:    "attach-event.json",
		ShareEventURL:  "detach-event.json",
	}
}

func parseEventDesc(ed string) (r shift.Resp, err error) {
	if len(ed) < 1 {
		return r, fmt.Errorf("event description is empty")
	}

	err = json.Unmarshal([]byte(ed), &r)
	return
}

func (gc *Config) GetCalendarEvents(s *service.Service, start, end int64) (shs []shift.Shift, err error) {
	c := calendar.NewClient(gc.BaseURL,
		gc.CreateEventURL,
		gc.DeleteEventURL,
		gc.GetEventsURL,
		gc.GetEventURL,
		gc.ShareEventURL,
	)
	loc, err := time.LoadLocation(s.Timezone)
	if err != nil {
		gc.Log.Errorf("calendarclient: LoadLocation returned %v", err)
		return
	}

	es, err := c.GetEventsInRange("1120000000081478", strconv.Itoa(s.Calendar.LayerID), s.Timezone, start, end)
	if err != nil {
		gc.Log.Errorf("GetEventsInRange returned %v", err)
		return
	}

	for _, e := range es {
		r, err := parseEventDesc(e.Description)
		if err != nil {
			gc.Log.Infof("Cannot parse event %d due to %v", e.EventID, err)
			continue
		}

		ts, err := time.ParseInLocation("2006-01-02T15:04:05", e.Start, loc)
		if err != nil {
			gc.Log.Infof("Cannot parse event %d due to %v", e.EventID, err)
			continue
		}

		te, err := time.ParseInLocation("2006-01-02T15:04:05", e.End, loc)
		if err != nil {
			gc.Log.Infof("Cannot parse event %d due to %v", e.EventID, err)
			continue
		}

		shs = append(shs, shift.Shift{
			ServiceID: s.ID.Hex(),
			DateStart: ts.Unix(),
			DateEnd:   te.Unix(),
			Resp:      r,
		})
	}

	return
}

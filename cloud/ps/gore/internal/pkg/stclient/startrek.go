package stclient

import (
	"fmt"
	"sort"
	"strconv"
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/shift"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/template"
	"a.yandex-team.ru/cloud/ps/gore/pkg/startrek"
	"a.yandex-team.ru/library/go/core/log"
)

type Storage interface {
	UpdateServiceByID(string, *service.Service) error
	GetShifts(int64, string, bool) ([]shift.Shift, error)
	ListTemplates(sid, tid, t string) (res []template.Template, err error)
	GetActiveTemplate(sid, t string) (res *template.Template, err error)
}

type Config struct {
	Log           log.Logger
	Storage       Storage
	BaseURL       string
	Issues        string
	DefaultValues string
	Transitions   string
	Queues        string
	Components    string
}

func DefaultConfig() *Config {
	return &Config{
		BaseURL:       "https://st-api.yandex-team.ru/v2/",
		Issues:        "issues",
		DefaultValues: "",
		Transitions:   "transitions",
		Queues:        "queues",
		Components:    "components/_suggest",
	}
}

type Auth struct {
	Method string
	KWArgs map[string]string
}

func (gc *Config) NewOAuth(t, u string) *Auth {
	return &Auth{
		Method: "OAuth",
		KWArgs: map[string]string{
			"token": t,
			"login": u,
		},
	}
}

func (gc *Config) Update(a *Auth, s *service.Service, shs []shift.Shift) {
	gc.Log.Info("Update on stcliemt.go")
	c := startrek.NewClient(gc.BaseURL,
		gc.Issues,
		gc.Transitions,
		gc.Queues,
		gc.Components,
	)
	switch a.Method {
	default:
	case "OAuth":
		c = c.SetOAuthToken(a.KWArgs["token"]).SetLogin(a.KWArgs["login"])
	}

	if *s.Startrack.Duty.Active {
		if err := gc.UpdateTicketForService(c, s, shs); err != nil {
			gc.Log.Errorf("Error: %v", err)
		}
	}

	if *s.Startrack.Template.Active {
		if err := gc.UpdateQueueValues(c, s, shs); err != nil {
			gc.Log.Errorf("Error: %v", err)
		}
	}
}

func (gc *Config) UpdateAssignee(c *startrek.Client, t string, shs []shift.Shift) (err error) {
	ticket := startrek.Ticket{}
	switch len(shs) {
	case 0:
		return
	case 1:
		break
	default:
		sort.Slice(shs, func(i, j int) bool {
			return shs[i].Resp.Order < shs[j].Resp.Order
		})
	}

	ticket.Assignee.Login = shs[0].Resp.Username
	_, err = c.UpdateTicket(t, ticket)

	return
}

func (gc *Config) CreateTicket(c *startrek.Client, s *service.Service, shs []shift.Shift, v *template.Variables) (t startrek.Ticket, err error) {
	l := len(shs)
	if l < 1 {
		return
	}

	sort.Slice(shs, func(i, j int) bool {
		return shs[i].Resp.Order < shs[j].Resp.Order
	})

	a := shs[0].Resp.Username
	f := []startrek.Person{}
	if l > 1 {
		for _, sh := range shs[1:] {
			f = append(f, startrek.Person{Login: sh.Resp.Username})
		}
	}

	t.Assignee.Login = a
	t.Followers = f
	t.Queue = &struct {
		Key string `json:"key,omitempty"`
	}{
		Key: s.Startrack.Duty.Queue,
	}
	t.Tags = []string{"duty-ticket"}

	v.User = c.Login
	if err := fillTemplatedData(s.ID.Hex(), gc.Storage, v, &t); err != nil {
		gc.Log.Errorf("Error: %v", err)
		return t, err
	}
	if len(s.Startrack.Duty.Last) > 0 {
		t.Links = append(t.Links, startrek.Link{Issue: s.Startrack.Duty.Last, Relationship: "relates"})
	}

	if s.Startrack.Duty.Component != "" {
		cps, err := c.GetComponents(s.Startrack.Duty.Component, s.Startrack.Duty.Queue)
		if err != nil {
			return t, err
		}

		if len(cps) > 0 {
			for _, cp := range cps {
				if cp.Name == s.Startrack.Duty.Component {
					t.Components = append(t.Components, startrek.Component{ID: strconv.Itoa(cp.ID)})
					break
				}
			}
		}
	}

	return c.CreateTicketForService(t)
}

func (gc *Config) CloseTicket(c *startrek.Client, t string) (err error) {
	if t == "" {
		return
	}

	v, err := c.CheckTransitions(t, "close")
	if err != nil {
		return err
	}

	if !v {
		return
	}
	return c.UpdateTransitions(t, "close") //TODO: need to check for valid transaction
}

var week = map[string]int{
	"Monday":    0,
	"Tuesday":   1,
	"Wednesday": 2,
	"Thursday":  3,
	"Friday":    4,
	"Saturday":  5,
	"Sunday":    6,
}

func (gc *Config) GetTicket(c *startrek.Client, t string) (startrek.Ticket, error) {
	//gc.Log.Debug("GetTicket func called")
	return c.GetTicket(t)
}

func (gc *Config) UpdateTicketForService(c *startrek.Client, s *service.Service, shs []shift.Shift) (err error) {
	//gc.Log.Debug("GetTicket func called")
	if !*s.Startrack.Duty.Creation.Managed {
		return gc.UpdateAssignee(c, s.Startrack.Duty.Last, shs)
	}

	loc, err := time.LoadLocation(s.Timezone)
	if err != nil {
		gc.Log.Errorf("Error: %v", err)
		return
	}

	now := time.Now()
	v := &template.Variables{
		Service: s.Service,
		Start:   now.In(loc),
	}

	ct := time.Date(1970, 1, 1, 0, 0, 0, 0, loc)
	if len(s.Startrack.Duty.Last) > 0 {
		t, err := gc.GetTicket(c, s.Startrack.Duty.Last)
		if err != nil {
			gc.Log.Errorf("Error: %v", err)
			return err
		}

		ct, err = time.ParseInLocation("2006-01-02T15:04:05-0700", t.CreatedAt, loc)
		if err != nil {
			gc.Log.Errorf("Error: %v", err)
			return err
		}
	}

	switch s.Startrack.Duty.Creation.Mode {
	case "schedule":
		// Rotate ticket every "rotation_hours" hours
		rotationHours, err := strconv.ParseInt(s.Startrack.Duty.Creation.KwArgs["rotation_hours"], 10, 32)
		if err != nil {
			gc.Log.Errorf("Error: %v", err)
			return err
		}

		// Create first ticket at "first_ticket_create_time", next every "rotation_hours"
		createAt, err := time.ParseInLocation("2006-01-02T15:04:05-0700", s.Startrack.Duty.Creation.KwArgs["first_ticket_create_time"], loc)
		if err != nil {
			gc.Log.Errorf("Error: %v", err)
			return err
		}

		n := now.Sub(createAt) / (time.Hour * time.Duration(rotationHours))

		when := createAt.Add(time.Hour * n * time.Duration(rotationHours))
		if ct.Before(when) && now.After(when) {
			v.End = when.Add(time.Hour * time.Duration(rotationHours)).In(loc)

			t, err := gc.CreateTicket(c, s, shs, v)
			if err != nil {
				gc.Log.Errorf("Error: %v", err)
				return err
			}

			if err = gc.CloseTicket(c, s.Startrack.Duty.Last); err != nil {
				gc.Log.Errorf("Error: %v", err)
				return err
			}

			s.Startrack.Duty.Last = t.Key
			return gc.Storage.UpdateServiceByID(s.ID.Hex(), s)
		}
		return gc.UpdateAssignee(c, s.Startrack.Duty.Last, shs)

	case "time":
		d := week[s.Startrack.Duty.Creation.KwArgs["weekday"]]
		d += int(time.Monday - now.Weekday())
		when := time.Date(now.Year(), now.Month(), now.Day(), 0, 0, 0, 0, loc).AddDate(0, 0, d)
		td, _ := time.ParseDuration(s.Startrack.Duty.Creation.KwArgs["duration"])
		when = when.Add(td)
		if ct.Before(when) && now.After(when) {
			v.End = now.Add(time.Hour * 24 * 7).In(loc)
			t, err := gc.CreateTicket(c, s, shs, v)
			if err != nil {
				gc.Log.Errorf("Error: %v", err)
				return err
			}

			if err = gc.CloseTicket(c, s.Startrack.Duty.Last); err != nil {
				gc.Log.Errorf("Error: %v", err)
				return err
			}

			s.Startrack.Duty.Last = t.Key
			return gc.Storage.UpdateServiceByID(s.ID.Hex(), s)
		}
		return gc.UpdateAssignee(c, s.Startrack.Duty.Last, shs)

	case "period":
		d, _ := strconv.ParseInt(s.Startrack.Duty.Creation.KwArgs["days"], 10, 32)
		ct = ct.Truncate(time.Minute * 30).Add(time.Hour * 24 * time.Duration(d))

		if now.After(ct) {
			v.End = now.Add(time.Hour * 24 * time.Duration(d)).In(loc)
			t, err := gc.CreateTicket(c, s, shs, v)
			if err != nil {
				gc.Log.Errorf("Error: %v", err)
				return err
			}

			err = gc.CloseTicket(c, s.Startrack.Duty.Last)
			if err != nil {
				gc.Log.Errorf("Error: %v", err)
				return err
			}

			s.Startrack.Duty.Last = t.Key
			return gc.Storage.UpdateServiceByID(s.ID.Hex(), s)
		}
		return gc.UpdateAssignee(c, s.Startrack.Duty.Last, shs)

	case "shift":
		stored, err := gc.Storage.GetShifts(ct.Unix(), s.ID.Hex(), false)
		if err != nil {
			gc.Log.Errorf("Error: %v", err)
			return err
		}

		stShift := shift.Shift{}
		for _, sh := range stored {
			ord, _ := strconv.Atoi(s.Startrack.Duty.Creation.KwArgs["order"])
			if sh.Resp.Order == ord {
				stShift = sh
				break
			}
		}

		// if stShift.DateStart == 0 {
		// 	return fmt.Errorf("found empty shift for ticket creation time. No actions will be performed")
		// }

		actual, err := gc.Storage.GetShifts(now.Unix(), s.ID.Hex(), false)
		if err != nil {
			gc.Log.Errorf("Error: %v", err)
			return err
		}

		acShift := shift.Shift{}
		for _, sh := range actual {
			ord, _ := strconv.Atoi(s.Startrack.Duty.Creation.KwArgs["order"])
			if sh.Resp.Order == ord {
				acShift = sh
				break
			}
		}

		if acShift.DateStart == 0 {
			return fmt.Errorf("found empty shift for ticket creation time. No actions will be performed")
		}

		if stShift == acShift {
			return nil
		}

		v.End = time.Unix(acShift.DateEnd, 0).In(loc)
		t, err := gc.CreateTicket(c, s, shs, v)
		if err != nil {
			gc.Log.Errorf("Error: %v", err)
			return err
		}

		err = gc.CloseTicket(c, s.Startrack.Duty.Last)
		if err != nil {
			gc.Log.Errorf("Error: %v", err)
			return err
		}

		s.Startrack.Duty.Last = t.Key
		return gc.Storage.UpdateServiceByID(s.ID.Hex(), s)

		// return gc.CloseTicket(c, s.Startrack.Duty.Last)
	}

	return
}

func (gc *Config) UpdateQueueValues(c *startrek.Client, s *service.Service, shs []shift.Shift) (err error) {
	l := len(shs)
	if l < 1 {
		return
	}

	sort.Slice(shs, func(i, j int) bool {
		return shs[i].Resp.Order < shs[j].Resp.Order
	})

	a := shs[0].Resp.Username
	f := []startrek.Person{}
	if l > 1 {
		for _, sh := range shs[1:] {
			f = append(f, startrek.Person{Login: sh.Resp.Username})
		}
	}

	q, err := c.GetQueueValues(s.Startrack.Template.Queue, s.Startrack.Template.RuleID)
	if err != nil {
		return
	}

	q.AppendFields = []string{"followers"}

	q.Values.Assignee = startrek.Person{Login: a}
	q.Values.Duty = []startrek.Person{{Login: a}}
	q.Values.Followers = f

	return c.UpdateQueueValues(q, s.Startrack.Template.Queue, s.Startrack.Template.RuleID)
}

func (gc *Config) Valid(kw map[string]string, mode string) (err error) {
	switch mode {
	default:
		return fmt.Errorf("unsupported option in 'mode' field")

	case "time":
		if _, ok := week[kw["weekday"]]; !ok {
			return fmt.Errorf("unsupported option in 'time.weekday' field")
		}
		_, err = time.ParseDuration(kw["duration"])
		if err != nil {
			return fmt.Errorf("unsupported option in 'time.duration' field")
		}
		return
	case "period":
		if _, err := strconv.Atoi(kw["days"]); err != nil {
			return fmt.Errorf("unsupported option in 'period'. '%s' should be an integer", kw["days"])
		}
		return

	case "shift":
		if _, err := strconv.Atoi(kw["order"]); err != nil {
			return fmt.Errorf("unsupported option in 'order'. '%s' should be an integer", kw["order"])
		}
		return
	}
}

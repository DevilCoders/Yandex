package jugglerclient

import (
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/shift"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/template"
	"a.yandex-team.ru/cloud/ps/gore/pkg/juggler"
	"a.yandex-team.ru/library/go/core/log"
	"github.com/imdario/mergo"
)

type Storage interface {
	GetActiveTemplate(serviceID, templateType string) (*template.Template, error)
}

type Config struct {
	Log        log.Logger
	Storage    Storage
	BaseURL    string
	ChecksBase string
	NotifySet  string
	NotifyGet  string
	EventsURL  string
}

func DefaultConfig() *Config {
	return &Config{
		BaseURL:    "http://juggler-api.search.yandex.net/api/",
		ChecksBase: "checks/list_checks",
		NotifySet:  "notify_rules/add_or_update_notify_rule",
		NotifyGet:  "notify_rules/get_notify_rules",
		EventsURL:  "http://juggler-push.search.yandex.net/events",
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
	gc.Log.Info("Update on jugglerclient.go")
	c := juggler.NewClient(
		gc.BaseURL,
		gc.ChecksBase,
		gc.NotifySet,
		gc.NotifyGet,
		gc.EventsURL,
	)
	switch a.Method {
	default:
	case "OAuth":
		c = c.SetOAuthToken(a.KWArgs["token"]).SetLogin(a.KWArgs["login"])
	}

	if err := gc.SetRulesForService(c, s, shs); err != nil {
		gc.Log.Errorf("Error: %v", err)
	}
}

func (gc *Config) SetRulesForService(c *juggler.Client, s *service.Service, shs []shift.Shift) (err error) {
	cookedResps := shift.FlattenShift(shift.FilterToCurrent(shs))
	if s.Juggler.TLPosition < 0 {
		s.Juggler.TLPosition = len(cookedResps)
	}

	if u, ok := s.TeamOwners["lead"]; ok {
		cookedResps = shift.AddUserToFlattenResps(cookedResps, u, s.Juggler.TLPosition)
	}

	if u, ok := s.TeamOwners["robot"]; ok {
		cookedResps = shift.AddUserToFlattenResps(cookedResps, u, len(cookedResps))
	}

	dt, _ := template.DefaultTemplate(DescriptionTemplate)
	if t, err := gc.Storage.GetActiveTemplate(s.ID.Hex(), DescriptionTemplate); err == nil {
		_ = mergo.Merge(&dt, t, mergo.WithOverride)
	}

	loc, err := time.LoadLocation(s.Timezone)
	if err != nil {
		gc.Log.Errorf("Error: %v", err)
		return
	}

	for name, id := range s.Juggler.Rules {
		r, err := c.GetRuleByID(id)
		if err != nil {
			gc.Log.Errorf("Error: %v", err)
			continue
		}

		v := &template.Variables{
			User:    c.Login,
			Env:     name,
			Service: s.Service,
			Start:   time.Now().In(loc),
		}
		d, err := dt.Render(v)
		if err != nil {
			gc.Log.Errorf("Error: %v", err)
			continue
		}

		if err = c.SetRuleOwners(r, d, s.Juggler.Namespace, cookedResps); err != nil {
			gc.Log.Errorf("Error: %v", err)
		}
	}
	return
}

package idmclient

import (
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/shift"
	"a.yandex-team.ru/cloud/ps/gore/pkg/idm"
	"a.yandex-team.ru/library/go/core/log"
)

type Config struct {
	Log         log.Logger
	BaseURL     string
	IdmGet      string
	IdmPost     string
	IdmSlugPath string
	IdmSystem   string
}

func DefaultConfig() *Config {
	return &Config{
		BaseURL:     "https://idm-api.yandex-team.ru/api/v1/",
		IdmGet:      "roles/",
		IdmPost:     "rolerequests/",
		IdmSlugPath: "/services/meta_experiments/cloud/development/ycproduction/duty_on_ycloud/*/",
		IdmSystem:   "abc",
	}
}

type Auth struct {
	Method string
	KWArgs map[string]string
}

func (gc *Config) NewOAuth(t string) *Auth {
	return &Auth{
		Method: "OAuth",
		KWArgs: map[string]string{"token": t},
	}
}

func (gc *Config) Update(a *Auth, s *service.Service, shs []shift.Shift) {
	gc.Log.Info("Update on idmclient.go")
	c := idm.NewClient(
		gc.BaseURL,
		gc.IdmGet,
		gc.IdmPost,
		gc.IdmSlugPath,
		gc.IdmSystem,
	)
	switch a.Method {
	default:
	case "OAuth":
		c = c.SetOAuthToken(a.KWArgs["token"])
	}

	if err := gc.SetIDMRolerForService(c, s, shs); err != nil {
		gc.Log.Errorf("Error: %v", err)
	}
}

//SetIDMRolerForService is a func
func (gc *Config) SetIDMRolerForService(c *idm.Client, s *service.Service, shs []shift.Shift) (err error) {
	for _, r := range s.IDM.Roles {
		for _, sh := range shs {
			t, err := c.GetTotalActiveRoles(r, sh.Resp.Username)
			if err != nil {
				gc.Log.Errorf("Error: %v", err)
				continue
			}

			if t < 1 {
				// CLOUD-80302. IDM role should have finish date on the next day after real shift end date
				idmRoleEndDate := sh.DateEnd + 24*int64(time.Hour.Seconds())
				err = c.SetRole(
					sh.Resp.Username,
					time.Unix(idmRoleEndDate, 0).Format("2006-01-02"),
					r,
				)
				if err != nil {
					gc.Log.Errorf("Error: %v", err)
					continue
				}
			}
		}
	}

	return
}

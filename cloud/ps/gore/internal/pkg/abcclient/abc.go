package abcclient

import (
	"container/list"
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/auth"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/shift"
	httpclient "a.yandex-team.ru/cloud/ps/gore/pkg/abc/http"
	"a.yandex-team.ru/library/go/core/log"
)

type Config struct {
	Log          log.Logger
	BaseURL      string
	GetShiftsURL string
}

const timeFormat = "2006-01-02T15:04:05-07:00"

func (gc *Config) NewOAuth(t, u string) *auth.Auth {
	return &auth.Auth{
		Method: auth.OAuth,
		OAuth: &auth.OAuthArgs{
			Token: t,
			Login: u,
		},
	}
}

func DefaultConfig() *Config {
	return &Config{
		BaseURL:      "https://abc-back.yandex-team.ru",
		GetShiftsURL: "api/v4/duty/shifts",
	}
}

func (gc *Config) Import(a *auth.Auth, s *service.Service) (shs []shift.Shift, err error) {
	c, _ := httpclient.NewClient(gc.BaseURL, gc.GetShiftsURL)
	c = c.Authenticate(auth.OAuth, a.OAuth.Token)
	now := time.Now()
	for _, sch := range s.Schedule.ABC.Schedule {
		if sch.Active == nil || !*sch.Active {
			continue
		}

		ashs, err := c.GetShifts(now.AddDate(0, -1, 0), now.AddDate(0, 1, 0), s.Schedule.ABC.ServiceID, sch.ID)
		if err != nil {
			gc.Log.Errorf("Error while loading shifts from schedule %d: %v", sch.ID, err)
			continue
		}

		queue := list.New()
		for _, ash := range ashs {
			if sch.FetchUnapproved != nil && !*sch.FetchUnapproved {
				if !ash.IsApproved {
					continue
				}
			}

			start, aErr := time.Parse(timeFormat, ash.Start)
			if aErr != nil {
				gc.Log.Errorf("Error while parsing shift %d: %v", ash.ID, aErr)
				continue
			}
			end, aErr := time.Parse(timeFormat, ash.End)
			if aErr != nil {
				gc.Log.Errorf("Error while parsing shift %d: %v", ash.ID, aErr)
				continue
			}

			var primaryOrBackup string
			if ash.IsPrimary == nil {
				// Backward compatibility with ABC Duties 1.0: use "order" field in service config
				primaryOrBackup = sch.Order[0]
			} else if *ash.IsPrimary {
				// ABC Duties 2.0: ignore "order" field in service config
				primaryOrBackup = "primary"
			} else {
				primaryOrBackup = "backup"
			}

			sh := shift.Shift{
				ServiceID: s.ID.Hex(),
				DateStart: start.Unix(),
				DateEnd:   end.Unix(),
				Resp: shift.Resp{
					Order:    s.Schedule.Order[primaryOrBackup],
					Username: ash.Person.Login,
				},
			}

			// TODO (andgein): Remove "Replaces" block after full migration to ABC Duties 2.0 (CLOUD-88641)
			if len(ash.Replaces) > 0 {
				var srs []shift.Shift
				for _, asr := range ash.Replaces {
					rstart, aErr := time.Parse(timeFormat, asr.Start)
					if aErr != nil {
						gc.Log.Errorf("Error while parsing shift %d: %v", asr.ID, aErr)
						continue
					}
					rend, aErr := time.Parse(timeFormat, asr.End)
					if aErr != nil {
						gc.Log.Errorf("Error while parsing shift %d: %v", asr.ID, aErr)
						continue
					}
					srs = append(srs,
						shift.Shift{
							ServiceID: s.ID.Hex(),
							DateStart: rstart.Unix(),
							DateEnd:   rend.Unix(),
							Resp: shift.Resp{
								Order:    s.Schedule.Order[primaryOrBackup],
								Username: asr.Person.Login,
							},
						})
				}
				shs = append(shs, shift.ApplyReplaces(&sh, srs)...)
			} else {
				shs = append(shs, sh)
			}

			// CLOUD-88641. If shift is created by ABC Duties 2.0 (aka Watcher), then don't squash shifts
			if ash.FromWatcher {
				continue
			}

			if sch.Squash == nil || !*sch.Squash {
				continue
			}

			index := 1 % len(sch.Order) // VERIFY: len(sch.Order)
			for e := queue.Front(); e != nil; e = e.Next() {
				prev := e.Value.(shift.Shift)
				prev.Resp = shift.Resp{
					Order:    s.Schedule.Order[sch.Order[index]],
					Username: ash.Person.Login,
				}
				shs = append(shs, prev)
				index = (index + 1) % len(sch.Order)
			}

			_ = queue.PushBack(shift.Shift{
				ServiceID: s.ID.Hex(),
				DateStart: start.Unix(),
				DateEnd:   end.Unix(),
				Resp:      shift.Resp{},
			})
			if queue.Len() > len(sch.Order)-1 {
				queue.Remove(queue.Front())
			}
		}

	}

	return
}

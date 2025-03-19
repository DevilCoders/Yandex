package csv

import (
	"encoding/csv"
	"io"
	"sort"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/shift"
	"a.yandex-team.ru/library/go/core/log"
)

type Config struct {
	Log log.Logger
}

func DefaultConfig() *Config {
	return &Config{}
}

type format struct {
	Date struct {
		Index int
		Form  string
	}
	Roles map[string][]int
}

// GetRespsCSV provides data extraction from plaintext csv files
func (gc *Config) GetRespsCSV(csvFile string, s *service.Service) (shs []shift.Shift, err error) {
	r := csv.NewReader(strings.NewReader(string(csvFile)))
	r.FieldsPerRecord = -1
	if s.Schedule.KwArgs["has_header"] == "true" {
		_, err = r.Read()
		if err != nil {
			gc.Log.Infof("%s: %v", s.Service, err)
			return
		}
	}

	if !strings.ContainsRune(s.Schedule.KwArgs["format"], ',') {
		err = ErrInvalidSchedule
		gc.Log.Errorf(ErrInvalidSchedule.Error())
		return
	}

	var f format
	f.Roles = make(map[string][]int)
	for index, field := range strings.Split(s.Schedule.KwArgs["format"], ",") {
		if strings.HasPrefix(field, "date:") {
			f.Date.Form = field[5:]
			f.Date.Index = index
		}
		if strings.HasPrefix(field, "staff:") {
			f.Roles[field[6:]] = append(f.Roles[field[6:]], index)
		}
	}

	if len(f.Date.Form) < 1 {
		err = ErrNoDateField
		gc.Log.Errorf(err.Error())
		return
	}

	loc, err := time.LoadLocation(s.Timezone)
	if err != nil {
		gc.Log.Errorf("Error: %v", err) // Cannot test
		return
	}

	f.Date.Form += "_15:04"

	rs := make([]shift.Shift, 0)
	for {
		buf, err := r.Read()
		if err == io.EOF {
			break
		} else if err != nil {
			gc.Log.Errorf("Error: %v", err)
			return shs, err
		}

		t, e := time.ParseInLocation(f.Date.Form, buf[f.Date.Index]+"_"+s.Schedule.KwArgs["time"], loc)
		if e != nil {
			gc.Log.Errorf("couldn't parse timestamp in line %v", e)
			continue
		}

		for role, order := range s.Schedule.Order {
			if indexes, ok := f.Roles[role]; ok {
				for _, i := range indexes {
					if len(buf) < i+1 {
						continue
					}

					if login := strings.TrimPrefix(buf[i], "staff:"); len(login) > 0 {
						rs = append(
							rs,
							shift.Shift{
								DateStart: t.Unix(),
								DateEnd:   0,
								Resp: shift.Resp{
									Order:    order,
									Username: login,
								},
							})
					}
				}
			}
		}
	}

	if len(rs) < 1 {
		return
	}

	sort.Slice(rs, func(i, j int) bool {
		return rs[i].DateStart < rs[j].DateStart
	})
	last := rs[len(rs)-1].DateStart
	done := map[shift.Shift]bool{}
	for _, order := range s.Schedule.Order {
		rs = append(
			rs,
			shift.Shift{
				DateStart: last + int64(time.Hour.Seconds())*24,
				Resp: shift.Resp{
					Order: order,
				},
			}, shift.Shift{
				DateStart: last + int64(time.Hour.Seconds())*24 + 1,
				Resp: shift.Resp{
					Order: order,
				},
			},
		)
		done[rs[len(rs)-1]] = true
		done[rs[len(rs)-2]] = true
	}

	for i, rsi := range rs {
		if done[rsi] {
			continue
		}

		cur := rsi.DateStart
		found := false
		for _, rsj := range rs[i:] {
			if cur != rsj.DateStart && found {
				shs = append(
					shs,
					shift.Shift{
						ServiceID: s.ID.Hex(),
						DateStart: rsi.DateStart,
						DateEnd:   cur,
						Resp:      rsi.Resp,
					})

				found = false
				break
			}

			if rsi.Resp.Order != rsj.Resp.Order {
				continue
			}

			if rsi.Resp == rsj.Resp {
				cur = rsj.DateStart
				done[rsj] = true
				found = false
				continue
			}

			if cur == rsj.DateStart {
				continue
			}

			found = true
			cur = rsj.DateStart
		}
	}

	return
}

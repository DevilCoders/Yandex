package api

import (
	"encoding/json"
	"errors"
	"net/http"
	"regexp"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"
	"github.com/imdario/mergo"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/shift"
)

var (
	ErrDutyIncorrectTZ = &APIErr{
		StatusCode: http.StatusBadRequest,
		Err:        errors.New("invalid timezone"),
	}
	ErrDutyIncorrectFrom = &APIErr{
		StatusCode: http.StatusBadRequest,
		Err:        errors.New("invalid 'from'"),
	}
	ErrDutyIncorrectTo = &APIErr{
		StatusCode: http.StatusBadRequest,
		Err:        errors.New("invalid 'to'"),
	}
	ErrDutyNotFound = &APIErr{
		StatusCode: http.StatusNotFound,
		Err:        errors.New("no actual shifts found"),
	}
	ErrDutyCreateInternal = &APIErr{
		StatusCode: http.StatusInternalServerError,
		Err:        errors.New("couldn't create shifts"),
	}
	ErrDutyDeleteInternal = &APIErr{
		StatusCode: http.StatusInternalServerError,
		Err:        errors.New("couldn't delete shifts"),
	}
	ErrDutyUpdateInternal = &APIErr{
		StatusCode: http.StatusInternalServerError,
		Err:        errors.New("couldn't update shifts"),
	}
)

// DutyAll - TODO
func (srv *Server) DutyAll(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	var start, end time.Time
	v := r.URL.Query()
	if len(v.Get("tz")) > 0 {
		loc, err := time.LoadLocation(v.Get("tz"))
		if err != nil {
			response(w, &status{
				Error:     ErrDutyIncorrectTZ,
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}

		if len(v.Get("from")) > 0 {
			start, err = time.ParseInLocation(apiTS, v.Get("from"), loc)
			if err != nil {
				response(w, &status{
					Error:     ErrDutyIncorrectFrom,
					RequestID: ctx.Value(middleware.RequestIDKey).(string),
				})
				return
			}
		}

		if len(v.Get("to")) > 0 {
			end, err = time.ParseInLocation(apiTS, v.Get("to"), loc)
			if err != nil {
				response(w, &status{
					Error:     ErrDutyIncorrectTo,
					RequestID: ctx.Value(middleware.RequestIDKey).(string),
				})
				return
			}
		}

	} else {
		var err error
		if len(v.Get("from")) > 0 {
			start, err = time.Parse(apiTS, v.Get("from"))
			if err != nil {
				response(w, &status{
					Error:     ErrDutyIncorrectFrom,
					RequestID: ctx.Value(middleware.RequestIDKey).(string),
				})
				return
			}
		}

		if len(v.Get("to")) > 0 {
			end, err = time.Parse(apiTS, v.Get("to"))
			if err != nil {
				response(w, &status{
					Error:     ErrDutyIncorrectTo,
					RequestID: ctx.Value(middleware.RequestIDKey).(string),
				})
				return
			}
		}
	}

	user := v.Get("user")
	sid := v.Get("sid")
	shs, err := srv.Config.Mongo.GetShiftsForInterval(start.Unix(), end.Unix(), sid, user, true)
	if err != nil {
		response(w, &status{
			Error:     ErrMalformedRequest,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
	} else {
		response(w, &status{
			Body: shs,
		})
	}
}

// Duty - TODO
func (srv *Server) Duty(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	var s *service.Service
	var err error
	sn := chi.URLParam(r, "serviceSlug")
	if m, _ := regexp.MatchString(objectIDRE, sn); m {
		// Probably ID
		s, err = srv.Config.Mongo.GetServiceByID(sn, nil)
	}

	if s == nil || err != nil {
		// Found nothing, trying slug
		s, err = srv.Config.Mongo.GetServiceByName(sn, nil)
	}

	if err != nil {
		response(w, &status{
			Error:     ErrServiceNotFound.SetMsg(sn),
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	loc, err := time.LoadLocation(s.Timezone)
	if err != nil {
		response(w, &status{
			Error:     ErrDutyIncorrectTZ,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	v := r.URL.Query()
	var start, end time.Time
	from := v.Get("from")
	if len(from) > 0 {
		start, err = time.ParseInLocation(apiTS, from, loc)
		if err != nil {
			response(w, &status{
				Error:     ErrDutyIncorrectFrom,
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}
	}

	to := v.Get("to")
	if len(to) > 0 {
		end, err = time.ParseInLocation(apiTS, to, loc)
		if err != nil {
			response(w, &status{
				Error:     ErrDutyIncorrectTo,
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}

	}

	user := v.Get("user")

	// Process "round_robin" mode by special way: select one of services and delegate duties to it
	if s.Schedule.Method == "round_robin" {
		shs, err := srv.processRoundRobinModeDuties(s, start, end, user)
		if err != nil {
			response(w, &status{
				Error:     ErrDutyCreateInternal,
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
		}
		response(w, &status{
			Body: shs,
		})
		return
	}

	shs, err := srv.Config.Mongo.GetShiftsForInterval(start.Unix(), end.Unix(), s.ID.Hex(), user, true)
	if err != nil {
		response(w, &status{
			Error:     ErrDutyCreateInternal,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
	} else {
		response(w, &status{
			Body: shs,
		})
	}
}

func (srv *Server) processRoundRobinModeDuties(s *service.Service, start time.Time, end time.Time, user string) (shifts []shift.Shift, err error) {
	var services []*service.Service
	for _, serviceSlug := range s.Schedule.RoundRobin.Services {
		childrenService, err := srv.Config.Mongo.GetServiceByName(serviceSlug, nil)
		if err != nil {
			return nil, err
		}
		services = append(services, childrenService)
	}

	currentTime := start
	for currentTime.Unix() <= end.Unix() {
		dayIndex := int(currentTime.Unix() / (60 * 60 * 24))
		periodIndex := dayIndex / s.Schedule.RoundRobin.PeriodDays
		currentService := services[periodIndex%len(s.Schedule.RoundRobin.Services)]
		shs, err := srv.Config.Mongo.GetShiftsForInterval(currentTime.Unix(), currentTime.Unix(), currentService.ID.Hex(), user, true)
		if err == nil {
			shifts = append(shifts, shs...)
		}
		currentTime = currentTime.Add(24 * time.Hour)
	}

	return shifts, nil
}

func (srv *Server) CreateDuty(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	s, err := srv.CheckAuthForService(r)
	if err != nil {
		response(w, &status{
			Error:     err,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	shs := []shift.Shift{}
	if err := json.NewDecoder(r.Body).Decode(&shs); err != nil {
		response(w, &status{
			Error:     ErrMalformedRequest,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	sid := s.ID.Hex()
	for _, sh := range shs {
		sh.ServiceID = sid
	}

	if err := srv.Config.Mongo.CreateShifts(shs); err != nil {
		response(w, &status{
			Error:     ErrDutyCreateInternal,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	response(w, &status{
		Status: "OK",
	})
}

func (srv *Server) DeleteDuty(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	if _, err := srv.CheckAuthForService(r); err != nil {
		response(w, &status{
			Error:     err,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	shids := []string{}
	if err := json.NewDecoder(r.Body).Decode(&shids); err != nil {
		response(w, &status{
			Error:     ErrMalformedRequest,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	if err := srv.Config.Mongo.DeleteShiftsByIDs(shids); err != nil {
		response(w, &status{
			Error:     ErrDutyDeleteInternal,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	response(w, &status{
		Status: "OK",
	})
}

func (srv *Server) UpdateDuty(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	s, err := srv.CheckAuthForService(r)
	if err != nil {
		response(w, &status{
			Error:     err,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	shs := []shift.Shift{}
	if id := r.URL.Query().Get("id"); len(id) > 0 {
		sh := new(shift.Shift)
		if err := json.NewDecoder(r.Body).Decode(sh); err != nil {
			response(w, &status{
				Error:     ErrMalformedRequest,
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}
		shs = append(shs, *sh)
	} else {
		if err := json.NewDecoder(r.Body).Decode(&shs); err != nil {
			response(w, &status{
				Error:     ErrMalformedRequest,
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}
	}

	for _, sh := range shs {
		shi, err := srv.Config.Mongo.GetShiftForServiceByID(s.ID.Hex(), sh.ID.Hex())
		if err != nil {
			continue
		}

		_ = mergo.Merge(shi, sh, mergo.WithOverride)
		if err = srv.Config.Mongo.UpdateShiftByID(sh.ID.Hex(), shi); err != nil {
			response(w, &status{
				Error:     ErrDutyCreateInternal,
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}
	}

	response(w, &status{
		Status: "OK",
	})
}

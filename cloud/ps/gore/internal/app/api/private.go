package api

import (
	"errors"
	"fmt"
	"net/http"
	"regexp"
	"strings"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/shift"
	"a.yandex-team.ru/cloud/ps/gore/pkg/bitbucket"
)

var (
	ErrImportInvalidSource = &APIErr{
		StatusCode: http.StatusConflict,
		Err:        errors.New("invalid source"),
	}
	ErrImportParse = &APIErr{
		StatusCode: http.StatusPreconditionFailed,
		Err:        errors.New("cannot parse data from source"),
	}
	ErrImportCannotStore = &APIErr{
		StatusCode: http.StatusInternalServerError,
		Err:        errors.New("could not write shifts to database"),
	}
)

// RenewAPIsForService - TODO
func (srv *Server) RenewAPIsForService(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	if err := checkAuthContext(ctx); err != nil {
		response(w, &status{
			Error:     err,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	var s *service.Service
	var err error
	if m, _ := regexp.MatchString(objectIDRE, chi.URLParam(r, "serviceSlug")); m {
		// Probably ID
		s, err = srv.Config.Mongo.GetServiceByID(chi.URLParam(r, "serviceSlug"), nil)
	}

	if s == nil || err != nil {
		// Found nothing, trying slug
		s, err = srv.Config.Mongo.GetServiceByName(chi.URLParam(r, "serviceSlug"), nil)
	}

	if err != nil {
		response(w, &status{
			Error:     ErrServiceNotFound,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	if err := checkPermissions(s, ctx.Value(CtxLogin).(string), ctx.Value(CtxScopes).([]string), appScopeWrite); err != nil {
		response(w, &status{
			Error:     ErrAuthForbidden,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	if s.Active == nil || !*s.Active {
		response(w, &status{
			Error:     ErrServiceInactive,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	t := time.Now().Unix()
	shs, err := srv.Config.Mongo.GetShiftsForInterval(t, t, s.ID.Hex(), "", true)
	if err != nil {
		response(w, &status{
			Error:     ErrDutyNotFound,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	//tmp := srv.Config.Mongo.
	// TODO: rewrite to goroutines
	if s.Juggler != nil && s.Juggler.IsActive() {
		shifts := shs
		if *s.Juggler.IncludeRest {
			if shifts, err = srv.Config.Mongo.GetShiftsForInterval(t, 0, s.ID.Hex(), "", true); err != nil {
				shifts = shs
			}
		}

		srv.Config.Juggler.Update(
			srv.Config.Juggler.NewOAuth(
				ctx.Value(CtxToken).(string),
				ctx.Value(CtxLogin).(string),
			),
			s,
			shifts,
		)
	}

	if s.IDM != nil && s.IDM.IsActive() {
		srv.Config.IDM.Update(srv.Config.IDM.NewOAuth(ctx.Value(CtxToken).(string)), s, shs)
	}

	if s.Startrack != nil && s.Startrack.IsActive() {
		srv.Config.Startrek.Update(
			srv.Config.Startrek.NewOAuth(
				ctx.Value(CtxToken).(string),
				ctx.Value(CtxLogin).(string),
			),
			s,
			shs,
		)
	}

	response(w, &status{
		Status: "OK",
	})
}

// RenewServiceData - TODO
func (srv *Server) RenewServiceData(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	if err := checkAuthContext(ctx); err != nil {
		response(w, &status{
			Error:     err,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	var s *service.Service
	var err error
	if m, _ := regexp.MatchString(objectIDRE, chi.URLParam(r, "serviceSlug")); m {
		// Probably ID
		s, err = srv.Config.Mongo.GetServiceByID(chi.URLParam(r, "serviceSlug"), nil)
	}

	if s == nil || err != nil {
		// Found nothing, trying slug
		s, err = srv.Config.Mongo.GetServiceByName(chi.URLParam(r, "serviceSlug"), nil)
	}

	if err != nil {
		response(w, &status{
			Error:     ErrServiceNotFound,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	if err := checkPermissions(s, ctx.Value(CtxLogin).(string), ctx.Value(CtxScopes).([]string), appScopeWrite); err != nil {
		response(w, &status{
			Error:     ErrAuthForbidden,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	m := strings.Split(s.Schedule.Method, ":")
	var resp string
	var shs []shift.Shift
	switch m[0] {
	default:
		response(w, &status{
			Error:     ErrImportInvalidSource.SetMsg(m[0]),
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return

	case "file":
		switch m[1] {
		default:
			response(w, &status{
				Error:     ErrImportInvalidSource.SetMsg(m[0]),
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return

		case "bitbucket":
			bb := bitbucket.NewClient(
				s.Schedule.KwArgs["url"],
				r.Context().Value(CtxToken).(string),
			)
			resp, err = bb.GetCSVFile()
			if err != nil {
				response(w, &status{
					Error:     ErrImportParse.SetMsg(err.Error()),
					RequestID: ctx.Value(middleware.RequestIDKey).(string),
				})
				return
			}

		case "github":
			resp = "git"
			err = nil
		}

		shs, err = srv.Config.CSV.GetRespsCSV(resp, s)
		if err != nil {
			response(w, &status{
				Error:     ErrImportParse.SetMsg(err.Error()),
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}

	case "calendar":
		shs, err = srv.Config.Calendar.GetCalendarEvents(s, time.Now().Unix(), time.Now().Add(30*24*time.Hour).Unix())
		if err != nil {
			response(w, &status{
				Error:     ErrImportParse.SetMsg(err.Error()),
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}

	case "abc":
		a := srv.Config.ABC.NewOAuth(r.Context().Value(CtxToken).(string), r.Context().Value(CtxLogin).(string))
		shs, err = srv.Config.ABC.Import(a, s) // DEBUG!!!!
		if err != nil {
			response(w, &status{
				Error:     ErrImportParse.SetMsg(err.Error()),
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}

	case "round_robin":
		// no-op, "round_robin" mode does not affect export and import, only /duty/... requests

	}

	err = srv.Config.Mongo.RenewShiftsForService(shs)
	if err != nil {
		response(w, &status{
			Error:     ErrImportCannotStore.SetMsg(err.Error()),
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	response(w, &status{
		Status: fmt.Sprintf("Shifts for service %s updated successfully", s.Service),
	})
}

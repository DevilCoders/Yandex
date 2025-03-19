package api

import (
	"encoding/json"
	"errors"
	"net/http"
	"regexp"
	"strings"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
)

var (
	ErrServiceNotFound = &APIErr{
		StatusCode: http.StatusBadRequest,
		Err:        errors.New("services not found"),
	}
	ErrServiceCreateInternal = &APIErr{
		StatusCode: http.StatusInternalServerError,
		Err:        errors.New("couldn't create service"),
	}
	ErrServiceDeleteInternal = &APIErr{
		StatusCode: http.StatusInternalServerError,
		Err:        errors.New("couldn't delete service"),
	}
	ErrServiceUpdateInternal = &APIErr{
		StatusCode: http.StatusInternalServerError,
		Err:        errors.New("couldn't update service"),
	}
	ErrServiceInvalidName = &APIErr{
		StatusCode: http.StatusBadRequest,
		Err:        errors.New("new slug doesnt match regex"),
	}
	ErrServiceConflict = &APIErr{
		StatusCode: http.StatusConflict,
		Err:        errors.New("service already exists"),
	}
	ErrServiceInactive = &APIErr{
		StatusCode: http.StatusBadRequest,
		Err:        errors.New("unable to update external APIs for inactive service"),
	}
)

func checkFields(fs string) (c []string) {
	if len(fs) > 0 {
		avail := service.JSONFields()
		for _, f := range strings.Split(fs, ",") {
			for _, a := range avail {
				if f == a {
					c = append(c, a)
					break
				}
			}
		}
	}
	return
}

// ServicesAll - TODO
func (srv *Server) ServicesAll(w http.ResponseWriter, r *http.Request) {
	c := checkFields(r.URL.Query().Get("fields"))
	if s, err := srv.Config.Mongo.GetServicesAll(c); err != nil {
		response(w, &status{
			Error:     ErrServiceNotFound,
			RequestID: r.Context().Value(middleware.RequestIDKey).(string),
		})
	} else {
		response(w, &status{
			Body: s,
		})
	}
}

// Service - TODO
func (srv *Server) Service(w http.ResponseWriter, r *http.Request) {
	c := checkFields(r.URL.Query().Get("fields"))
	sn := chi.URLParam(r, "serviceSlug")
	var s *service.Service
	var err error
	if m, _ := regexp.MatchString(objectIDRE, sn); m {
		// Probably ID
		s, err = srv.Config.Mongo.GetServiceByID(sn, c)
	}

	if s == nil || err != nil {
		// Found nothing, trying slug
		s, err = srv.Config.Mongo.GetServiceByName(sn, c)
	}

	if err != nil {
		response(w, &status{
			Error:     ErrServiceNotFound.SetMsg(sn),
			RequestID: r.Context().Value(middleware.RequestIDKey).(string),
		})
	} else {
		response(w, &status{
			Body: s,
		})
	}
}

// ServiceUpdate - TODO
func (srv *Server) ServiceUpdate(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	s := new(service.Service)
	err := json.NewDecoder(r.Body).Decode(s)
	if err != nil {
		response(w, &status{
			Error:     ErrMalformedRequest,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	if len(s.Service) > 0 {
		matched, _ := regexp.MatchString(slugRE, s.Service) //TODO: rewrite to full check
		if !matched {
			response(w, &status{
				Error:     ErrServiceInvalidName.SetMsg(slugRE),
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}

		if _, err := srv.Config.Mongo.GetServiceByName(s.Service, []string{"service"}); err == nil {
			response(w, &status{
				Error:     ErrServiceConflict.SetMsg(s.Service),
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}
	}

	_, e := srv.CheckAuthForService(r)
	if e != nil {
		response(w, &status{
			Error:     e,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	sn := chi.URLParam(r, "serviceSlug")
	if m, _ := regexp.MatchString(objectIDRE, sn); m {
		// Probably ID
		err = srv.Config.Mongo.UpdateServiceByID(sn, s)
		if err != nil {
			// Unchanged, trying slug
			err = srv.Config.Mongo.UpdateServiceByName(sn, s)
		}
	} else {
		err = srv.Config.Mongo.UpdateServiceByName(sn, s)
	}

	if err != nil {
		response(w, &status{
			Error:     ErrServiceUpdateInternal,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
	} else {
		response(w, &status{
			Status: "OK",
		})
	}
}

// ServiceSet - TODO
func (srv *Server) ServiceSet(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	if e := checkAuthContext(ctx); e != nil {
		response(w, &status{
			Error:     e,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	if !inScopes(ctx.Value(CtxScopes).([]string), appScopeWrite) {
		response(w, &status{
			Error:     ErrAuthForbidden.SetMsg("no active permission for token"),
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	s := new(service.Service)
	if err := json.NewDecoder(r.Body).Decode(s); err != nil {
		response(w, &status{
			Error:     ErrMalformedRequest,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	if matched, _ := regexp.MatchString(slugRE, s.Service); !matched {
		response(w, &status{
			Error:     ErrServiceInvalidName,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	if _, err := srv.Config.Mongo.GetServiceByID(s.ID.Hex(), nil); err == nil {
		response(w, &status{
			Error:     ErrServiceConflict.SetMsg(s.Service),
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	if err := srv.Config.Mongo.CreateService(s); err != nil {
		response(w, &status{
			Error:     ErrServiceCreateInternal,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}
	response(w, &status{
		Status: "OK",
	})
}

// ServiceDelete - TODO
func (srv *Server) ServiceDelete(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	if _, e := srv.CheckAuthForService(r); e != nil {
		response(w, &status{
			Error:     e,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	var err error
	sn := chi.URLParam(r, "serviceSlug")
	if m, _ := regexp.MatchString(objectIDRE, sn); m {
		// Probably ID
		err = srv.Config.Mongo.DeleteServiceByID(sn)
		if err != nil {
			// Unchanged, trying slug
			err = srv.Config.Mongo.DeleteServiceByName(sn)
		}
	} else {
		err = srv.Config.Mongo.DeleteServiceByName(sn)
	}

	if err != nil {
		response(w, &status{
			Error:     ErrServiceDeleteInternal,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
	} else {
		response(w, &status{
			Status: "OK",
		})
	}
}

package api

import (
	"encoding/json"
	"errors"
	"net/http"
	"regexp"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/template"
	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"
)

var (
	ErrNoTemplates = &APIErr{
		StatusCode: http.StatusNotFound,
		Err:        errors.New("no templates found"),
	}
	ErrTemplateNotFound = &APIErr{
		StatusCode: http.StatusNotFound,
		Err:        errors.New("couldn't create templates"),
	}
	ErrTemplateIncorrectFrom = &APIErr{
		StatusCode: http.StatusBadRequest,
		Err:        errors.New("invalid 'from'"),
	}
	ErrTemplateIncorrectTo = &APIErr{
		StatusCode: http.StatusBadRequest,
		Err:        errors.New("invalid 'to'"),
	}
	ErrTemplateCreateInternal = &APIErr{
		StatusCode: http.StatusInternalServerError,
		Err:        errors.New("couldn't create templates"),
	}
	ErrTemplateDeleteInternal = &APIErr{
		StatusCode: http.StatusInternalServerError,
		Err:        errors.New("couldn't delete templates"),
	}
	ErrTemplateUpdateInternal = &APIErr{
		StatusCode: http.StatusInternalServerError,
		Err:        errors.New("couldn't update templates"),
	}
	ErrTemplateConflict = &APIErr{
		StatusCode: http.StatusBadRequest,
		Err:        errors.New("some of templates provided are unique per service and already exist"),
	}
)

func (srv *Server) Template(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	sn := chi.URLParam(r, "serviceSlug")
	var s *service.Service
	var err error
	if m, _ := regexp.MatchString(objectIDRE, sn); m {
		s, err = srv.Config.Mongo.GetServiceByID(sn, []string{"service"})
	}

	if s == nil || err != nil {
		s, err = srv.Config.Mongo.GetServiceByName(sn, []string{"service"})
	}

	if err != nil {
		response(w, &status{
			Error:     ErrServiceNotFound,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	v := r.URL.Query()
	if t, err := srv.Config.Mongo.ListTemplates(s.ID.Hex(), v.Get("id"), v.Get("type")); err != nil {
		response(w, &status{
			Error:     ErrNoTemplates,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
	} else {
		response(w, &status{
			Body: t,
		})
	}

}

func (srv *Server) DeleteTemplates(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	if _, e := srv.CheckAuthForService(r); e != nil {
		response(w, &status{
			Error:     e,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	tids := []string{}
	if err := json.NewDecoder(r.Body).Decode(&tids); err != nil {
		response(w, &status{
			Error:     ErrMalformedRequest,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	if err := srv.Config.Mongo.DeleteTemplatesByIDs(tids); err != nil {
		response(w, &status{
			Error:     ErrTemplateDeleteInternal,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
	} else {
		response(w, &status{
			Status: "OK",
		})
	}
}

func (srv *Server) CreateTemplates(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	s, e := srv.CheckAuthForService(r)
	if e != nil {
		response(w, &status{
			Error:     e,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	ts := []template.Template{}
	if err := json.NewDecoder(r.Body).Decode(&ts); err != nil {
		response(w, &status{
			Error:     ErrMalformedRequest,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	fts := []template.Template{}
	sid := s.ID.Hex()
	for _, t := range ts {
		if dt, err := template.DefaultTemplate(t.Type); err == nil {
			t.Unique = dt.Unique
			t.ServiceID = sid
			fts = append(fts, t)
		}
	}

	if ct, err := srv.Config.Mongo.CheckForTemplateConflicts(fts); err != nil {
		response(w, &status{
			Error:     ErrTemplateCreateInternal,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	} else if ct != nil {
		response(w, &status{
			Body:      ct,
			Error:     ErrTemplateConflict,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	if err := srv.Config.Mongo.CreateTemplates(fts); err != nil {
		response(w, &status{
			Error:     ErrTemplateCreateInternal,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
	} else {
		response(w, &status{
			Status: "OK",
		})
	}
}

func (srv *Server) UpdateTemplates(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	s, e := srv.CheckAuthForService(r)
	if e != nil {
		response(w, &status{
			Error:     e,
			RequestID: ctx.Value(middleware.RequestIDKey).(string),
		})
		return
	}

	ts := []template.Template{}
	if id := r.URL.Query().Get("id"); len(id) > 0 {
		t := template.Template{}
		if err := json.NewDecoder(r.Body).Decode(&t); err != nil {
			response(w, &status{
				Error:     ErrMalformedRequest,
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}
		ts = append(ts, t)
	} else {
		if err := json.NewDecoder(r.Body).Decode(&ts); err != nil {
			response(w, &status{
				Error:     ErrMalformedRequest,
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}
	}

	sid := s.ID.Hex()
	for i := range ts {
		if dt, err := template.DefaultTemplate(ts[i].Type); err == nil {
			ts[i].Unique = dt.Unique
			ts[i].ServiceID = sid
		}
	}

	for _, t := range ts {
		if err := srv.Config.Mongo.UpdateTemplate(t.ID.Hex(), &t); err != nil {
			response(w, &status{
				Error:     ErrTemplateUpdateInternal,
				RequestID: ctx.Value(middleware.RequestIDKey).(string),
			})
			return
		}
	}

	response(w, &status{
		Status: "OK",
	})
}

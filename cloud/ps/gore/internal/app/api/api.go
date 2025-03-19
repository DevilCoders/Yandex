package api

import (
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"

	"a.yandex-team.ru/cloud/ps/gore/internal/app/config"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
)

const (
	appScopeWrite = "yc_resps:write"
	appScopeRead  = "yc_resps:read"
)

var (
	ErrMalformedRequest = &APIErr{
		StatusCode: http.StatusBadRequest,
		Err:        errors.New("malformed request"),
	}
)

type status struct {
	Error     error       `json:"error,omitempty"`
	Status    string      `json:"status,omitempty"`
	RequestID string      `json:"requestID,omitempty"`
	Body      interface{} `json:"-"`
}

func response(w http.ResponseWriter, s *status) {
	var e *APIErr
	if errors.As(s.Error, &e) {
		w.WriteHeader(s.Error.(*APIErr).StatusCode)
	} else {
		w.WriteHeader(http.StatusOK)
	}

	enc := json.NewEncoder(w)
	enc.SetIndent("", "  ")
	if s.Body != nil {
		_ = enc.Encode(s.Body)
	} else {
		_ = enc.Encode(s)
	}
}

type Server struct {
	BaseLogger *zap.Logger
	Config     *config.Config
	BBClient   *httpbb.Client
}

const (
	apiTS      = "02-01-2006T15:04"
	slugRE     = `[0-9a-zA-Z][0-9a-zA-Z_-]{0,30}[0-9a-zA-Z]`
	objectIDRE = `[0-9a-zA-Z]{24}`
)

func privateAPI(srv *Server) chi.Router {
	r := chi.NewRouter()
	r.Route("/", func(r chi.Router) {
		r.Use(middleware.SetHeader("Content-Type", "application/json"))
		r.Use(middleware.RealIP)
		r.Use(middleware.RequestID)
		r.Use(srv.mwLogger())
		r.Use(srv.mwOAuth(srv.BBClient))
		r.Use(middleware.StripSlashes)
		r.Post("/services", srv.ServiceSet)
		r.Get("/services", srv.ServicesAll)
		r.Route(fmt.Sprintf("/services/{serviceSlug:%s}", slugRE), func(r chi.Router) {
			r.With(srv.cache(10*time.Second)).Get("/", srv.Service)
			r.Get("/", srv.Service)
			r.Put("/", srv.ServiceUpdate)
			r.Delete("/", srv.ServiceDelete)
		})
		r.Get("/users", srv.UsersAll)
		r.Route("/users/{userSlug}", func(r chi.Router) {
			r.Get("/", srv.User)
		})
		r.Route(fmt.Sprintf("/import/{serviceSlug:%s}", slugRE), func(r chi.Router) {
			r.Post("/", srv.RenewServiceData)
		})
		r.Route(fmt.Sprintf("/export/{serviceSlug:%s}", slugRE), func(r chi.Router) {
			r.Post("/", srv.RenewAPIsForService)
		})
	})

	return r
}

func publicAPI(srv *Server) chi.Router {
	r := chi.NewRouter()
	r.Route("/", func(r chi.Router) {
		r.Use(middleware.SetHeader("Content-Type", "application/json"))
		r.Use(middleware.RealIP)
		r.Use(middleware.RequestID)
		r.Use(srv.mwLogger())
		r.Use(srv.mwOAuth(srv.BBClient))
		r.Use(middleware.StripSlashes)
		r.Post("/services", srv.ServiceSet)
		r.Get("/services", srv.ServicesAll)
		r.Route(fmt.Sprintf("/services/{serviceSlug:%s}", slugRE), func(r chi.Router) {
			r.Get("/", srv.Service)
			r.Patch("/", srv.ServiceUpdate)
			r.Delete("/", srv.ServiceDelete)
		})
		r.Get("/users", srv.UsersAll)
		r.Route("/users/{userSlug}", func(r chi.Router) {
			r.Get("/", srv.User)
		})
		r.Get("/duty", srv.DutyAll)
		r.Route(fmt.Sprintf("/duty/{serviceSlug:%s}", slugRE), func(r chi.Router) {
			r.Get("/", srv.Duty)
			r.Post("/", srv.CreateDuty)
			r.Patch("/", srv.UpdateDuty)
			r.Delete("/", srv.DeleteDuty)
		})
		r.Route(fmt.Sprintf("/templates/{serviceSlug:%s}", slugRE), func(r chi.Router) {
			r.Get("/", srv.Template)
			r.Post("/", srv.CreateTemplates)
			r.Patch("/", srv.UpdateTemplates)
			r.Delete("/", srv.DeleteTemplates)
		})
	})
	return r
}

// Mux - TODO
func Mux(srv *Server) {
	r := chi.NewRouter()
	r.Get("/ping", func(w http.ResponseWriter, r *http.Request) {
		if _, err := w.Write([]byte("pong")); err != nil {
			srv.BaseLogger.Debugf("Unable to pong: %v", err)
		}
	})

	r.Mount("/api/v0private", privateAPI(srv))
	r.Mount("/api/v0", publicAPI(srv))
	////
	// walkFunc := func(method string, route string, handler http.Handler, middlewares ...func(http.Handler) http.Handler) error {
	// 	route = strings.Replace(route, "/*/", "/", -1)
	// 	fmt.Printf("%s %s\n", method, route)
	// 	return nil
	// }

	// if err := chi.Walk(r, walkFunc); err != nil {
	// 	fmt.Printf("Logging err: %s\n", err.Error())
	// }
	///
	if err := http.ListenAndServe(":8080", r); err != nil {
		srv.BaseLogger.Debugf("Server stopped: %v", err)
	}
}

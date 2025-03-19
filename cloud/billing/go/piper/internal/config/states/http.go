package states

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"sort"
	"strings"
	"sync"
	"time"

	"github.com/go-chi/chi/v5"
)

const (
	httpOkBody = `{"ok":true}
`
	jugglerOkBody = `{"events":[{"service":"piper-health","status":"OK"}]
`
	shutdownBody = `{"ok":false,"reason":"shutingdown"}
`
	shutdownJugglerBody = `{"events":[{"service":"piper-health","status":"CRIT"}]
`
)

func (m *Manager) RouteHealthCheck(r chi.Router) {
	r.Use(m.shutdownMiddleware(shutdownBody))
	r.Get("/", m.handleCriticalHealthChecks)
	r.Get("/*", m.handleHealthCeck)
}

func (m *Manager) RouteJugglerCheck(r chi.Router) {
	r.Use(m.shutdownMiddleware(shutdownJugglerBody))
	r.Get("/", m.handleJugglerCheck)
}

func (m *Manager) RouteStats(r chi.Router) {
	r.Use(m.shutdownMiddleware(shutdownBody))
	r.Get("/", m.handleStatsList)
	r.Get("/*", m.handleStats)
}

func (m *Manager) shutdownMiddleware(body string) func(http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if !m.closed {
				next.ServeHTTP(w, r)
				return
			}
			w.Header().Set("Content-Type", "application/json")
			w.WriteHeader(http.StatusServiceUnavailable)
			_, _ = w.Write([]byte(body))
		})
	}
}

func (m *Manager) handleHealthCeck(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	jsEnc := json.NewEncoder(w)
	name := chi.URLParam(r, "*")
	check := m.getHealthChecker(name)
	if check == nil {
		w.WriteHeader(http.StatusNotFound)
		_ = jsEnc.Encode(healthCheckStatusResponse{Name: name, Error: "health check not found"})
		return
	}
	err := check.HealthCheck(r.Context())
	status := http.StatusOK
	resp := healthCheckStatusResponse{Name: name, Ok: err == nil}
	if err != nil {
		resp.Error = err.Error()
		status = http.StatusInternalServerError
	}
	w.WriteHeader(status)
	_ = jsEnc.Encode(resp)
}

func (m *Manager) handleCriticalHealthChecks(w http.ResponseWriter, r *http.Request) {
	hc, names := m.getCritHealthCheckers()
	w.Header().Set("Content-Type", "application/json")
	if len(hc) == 0 {
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write([]byte(httpOkBody))
		return
	}

	waitCtx, cancel := context.WithTimeout(r.Context(), time.Second)

	hcErrors := make([]error, len(hc))
	go func() { // run healthcheck async for force timeout
		defer cancel()
		// TODO: recover
		wg := sync.WaitGroup{}
		for i := range hc {
			i, c := i, hc[i]
			wg.Add(1)
			go func() {
				defer wg.Done()
				hcErrors[i] = c.HealthCheck(waitCtx)
			}()
		}
		wg.Wait()
	}()

	<-waitCtx.Done()
	jsEnc := json.NewEncoder(w)
	if errors.Is(waitCtx.Err(), context.DeadlineExceeded) || r.Context().Err() != nil { // timeout or client cancel
		w.WriteHeader(http.StatusGatewayTimeout)
		_ = jsEnc.Encode(healthChecksResponse{Ok: false, Reason: "check canceled"})
		return
	}

	resp := healthChecksResponse{Ok: true}
	for i := range hcErrors {
		err := hcErrors[i]
		errStr := ""
		if err != nil {
			errStr = err.Error()
		}
		resp.Status = append(resp.Status, healthCheckStatusResponse{
			Name:  names[i],
			Ok:    err == nil,
			Error: errStr,
		})
		if resp.Ok && err != nil {
			resp.Ok = false
			resp.Reason = "error in critical system"
		}
	}

	status := http.StatusOK
	if !resp.Ok {
		status = http.StatusInternalServerError
	}

	w.WriteHeader(status)
	_ = jsEnc.Encode(resp)
}

func (m *Manager) handleJugglerCheck(w http.ResponseWriter, r *http.Request) {
	hc, names, critical := m.getAllHealthCheckers()
	w.Header().Set("Content-Type", "application/json")
	if len(hc) == 0 {
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write([]byte(jugglerOkBody))
		return
	}

	waitCtx, cancel := context.WithTimeout(r.Context(), time.Second)

	hcErrors := make([]error, len(hc))
	go func() { // run healthcheck async for force timeout
		defer cancel()
		// TODO: recover
		wg := sync.WaitGroup{}
		for i := range hc {
			i, c := i, hc[i]
			wg.Add(1)
			go func() {
				defer wg.Done()
				hcErrors[i] = c.HealthCheck(waitCtx)
			}()
		}
		wg.Wait()
	}()

	<-waitCtx.Done()
	jsEnc := json.NewEncoder(w)
	if errors.Is(waitCtx.Err(), context.DeadlineExceeded) || r.Context().Err() != nil { // timeout or client cancel
		w.WriteHeader(http.StatusGatewayTimeout)
		_ = jsEnc.Encode(jugglerResponse{Events: []jugglerCheck{
			{Service: "piper-health", Status: "WARN", Description: "timeout"},
		}})
		return
	}

	criticalNames := make(map[string]bool, len(critical))
	for _, cn := range critical {
		criticalNames[cn] = true
	}

	eventsMap := map[string]jugglerCheck{}
	healthEvent := jugglerCheck{
		Service:     "piper-health",
		Status:      "OK",
		Description: "ok",
	}
	for i := range hcErrors {
		name := names[i]
		service := fmt.Sprintf("piper-health-%s", strings.SplitN(name, ":", 2)[0])

		event := eventsMap[service]
		if event.Status == "CRIT" {
			continue
		}

		err := hcErrors[i]
		event.Service = service
		event.Status = "OK"
		event.Description = "ok"
		if err != nil {
			event.Status = "CRIT"
			event.Description = err.Error()
		}
		eventsMap[service] = event

		if criticalNames[name] && err != nil && healthEvent.Status != "CRIT" {
			healthEvent.Status = "CRIT"
			healthEvent.Description = "error in critical system"
		}
	}
	services := make([]string, 0, len(eventsMap))
	for service := range eventsMap {
		services = append(services, service)
	}
	sort.Strings(services)

	events := make([]jugglerCheck, 1, len(eventsMap)+1)
	events[0] = healthEvent
	for _, service := range services {
		events = append(events, eventsMap[service])
	}

	status := http.StatusOK
	w.WriteHeader(status)
	_ = jsEnc.Encode(jugglerResponse{Events: events})
}

func (m *Manager) handleStats(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	jsEnc := json.NewEncoder(w)
	name := chi.URLParam(r, "*")
	sp := m.getStatsProvider(name)
	if sp == nil {
		w.WriteHeader(http.StatusNotFound)
		_ = jsEnc.Encode(struct{}{})
		return
	}
	stats := sp()
	w.WriteHeader(http.StatusOK)
	_ = jsEnc.Encode(stats)
}

func (m *Manager) handleStatsList(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")

	type providers struct {
		Providers []string
	}

	data, _ := json.Marshal(providers{m.listStatsProviderNames()})

	_, _ = w.Write(data)
}

type healthChecksResponse struct {
	Ok     bool                        `json:"ok"`
	Reason string                      `json:"reason,omitempty"`
	Status []healthCheckStatusResponse `json:"status,omitempty"`
}

type healthCheckStatusResponse struct {
	Name  string `json:"name"`
	Ok    bool   `json:"ok,omitempty"`
	Error string `json:"error,omitempty"`
}

type jugglerResponse struct {
	Events []jugglerCheck `json:"events"`
}

type jugglerCheck struct {
	Service     string `json:"service"`
	Status      string `json:"status"`
	Description string `json:"description"`
}

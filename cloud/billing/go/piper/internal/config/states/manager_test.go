package states

import (
	"context"
	"errors"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/go-chi/chi/v5"
	"github.com/stretchr/testify/suite"
)

type managerTestSute struct {
	suite.Suite

	manager Manager

	healthCheckErr error
	close          closeCall
	stats          interface{}
}

func TestManager(t *testing.T) {
	suite.Run(t, new(managerTestSute))
}

func (suite *managerTestSute) SetupTest() {
	suite.manager = Manager{}
	suite.healthCheckErr = nil
	suite.close = nil
	suite.stats = nil
}

func (suite *managerTestSute) TestAdd() {
	suite.manager.Add("test_suite", suite)
	suite.Contains(suite.manager.healthCheckers, "test_suite")
	suite.Empty(suite.manager.criticalHealthCheckers)
	suite.Len(suite.manager.statsProviders, 1)
	suite.Len(suite.manager.closers, 1)
}

func (suite *managerTestSute) TestAddCritical() {
	suite.manager.AddCritical("test_suite", suite)
	suite.Contains(suite.manager.healthCheckers, "test_suite")
	suite.Contains(suite.manager.criticalHealthCheckers, "test_suite")
	suite.Len(suite.manager.criticalHealthCheckers, 1)
	suite.Len(suite.manager.statsProviders, 1)
	suite.Len(suite.manager.closers, 1)
}

func (suite *managerTestSute) TestHTTPCriticalCheckEmpty() {
	hndl := chi.NewMux()
	hndl.Route("/ping", suite.manager.RouteHealthCheck)

	r := httptest.NewRequest("GET", "/ping", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusOK, w.Result().StatusCode, w.Body.String())
}

func (suite *managerTestSute) TestHTTPCriticalCheckOk() {
	suite.manager.AddCritical("test_suite", suite)

	hndl := chi.NewMux()
	hndl.Route("/ping", suite.manager.RouteHealthCheck)

	r := httptest.NewRequest("GET", "/ping", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusOK, w.Result().StatusCode, w.Body.String())
}

func (suite *managerTestSute) TestHTTPCriticalCheckErr() {
	suite.manager.AddCritical("test_suite", suite)
	suite.healthCheckErr = errors.New("hc err")

	hndl := chi.NewMux()
	hndl.Route("/ping", suite.manager.RouteHealthCheck)

	r := httptest.NewRequest("GET", "/ping", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusInternalServerError, w.Result().StatusCode, w.Body.String())
}

func (suite *managerTestSute) TestHTTPHealthCheckOk() {
	suite.manager.Add("test_suite", suite)

	hndl := chi.NewMux()
	hndl.Route("/ping", suite.manager.RouteHealthCheck)

	r := httptest.NewRequest("GET", "/ping/test_suite", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusOK, w.Result().StatusCode, w.Body.String())
}

func (suite *managerTestSute) TestHTTPHealthCheckErr() {
	suite.manager.Add("test_suite", suite)
	suite.healthCheckErr = errors.New("hc err")

	hndl := chi.NewMux()
	hndl.Route("/ping", suite.manager.RouteHealthCheck)

	r := httptest.NewRequest("GET", "/ping/test_suite", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusInternalServerError, w.Result().StatusCode, w.Body.String())
}

func (suite *managerTestSute) TestHTTPHealthCheckNotFound() {
	hndl := chi.NewMux()
	hndl.Route("/ping", suite.manager.RouteHealthCheck)

	r := httptest.NewRequest("GET", "/ping/test_suite", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusNotFound, w.Result().StatusCode, w.Body.String())
}

func (suite *managerTestSute) TestHTTPHealthCheckWithSlashes() {
	suite.manager.Add("test:suite/with/slashes", suite)

	hndl := chi.NewMux()
	hndl.Route("/ping", suite.manager.RouteHealthCheck)

	r := httptest.NewRequest("GET", "/ping/test:suite/with/slashes", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusOK, w.Result().StatusCode, w.Body.String())
}

func (suite *managerTestSute) TestHTTPJuggler() {
	suite.manager.Add("test_suite", suite)
	suite.manager.Add("with_instance:/this/is.instance:1", suite)

	hndl := chi.NewMux()
	hndl.Route("/juggler", suite.manager.RouteJugglerCheck)

	r := httptest.NewRequest("GET", "/juggler", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusOK, w.Result().StatusCode, w.Body.String())
	suite.Require().JSONEq(
		`{"events":[
			{"description":"ok","service":"piper-health","status":"OK"},
			{"description":"ok","service":"piper-health-test_suite","status":"OK"},
			{"description":"ok","service":"piper-health-with_instance","status":"OK"}
		]}`,
		w.Body.String(),
	)
}

func (suite *managerTestSute) TestHTTPJugglerErr() {
	suite.manager.Add("test_suite", suite)
	suite.healthCheckErr = errors.New("hc err")

	hndl := chi.NewMux()
	hndl.Route("/juggler", suite.manager.RouteJugglerCheck)

	r := httptest.NewRequest("GET", "/juggler", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusOK, w.Result().StatusCode, w.Body.String())
	suite.Require().JSONEq(
		`{"events":[
			{"service":"piper-health","status":"OK", "description": "ok"},
			{"service":"piper-health-test_suite","status":"CRIT","description": "hc err"}
		]}`,
		w.Body.String(),
	)
}

func (suite *managerTestSute) TestHTTPJugglerCritErr() {
	suite.manager.AddCritical("test_suite", suite)
	suite.healthCheckErr = errors.New("hc err")

	hndl := chi.NewMux()
	hndl.Route("/juggler", suite.manager.RouteJugglerCheck)

	r := httptest.NewRequest("GET", "/juggler", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusOK, w.Result().StatusCode, w.Body.String())
	suite.Require().JSONEq(
		`{"events":[
			{"service":"piper-health","status":"CRIT", "description":"error in critical system"},
			{"service":"piper-health-test_suite","status":"CRIT","description": "hc err"}
		]}`,
		w.Body.String(),
	)
}

func (suite *managerTestSute) TestHTTPStats() {
	suite.manager.Add("test_suite", suite)
	suite.stats = "this_is_stats"

	hndl := chi.NewMux()
	hndl.Route("/stats", suite.manager.RouteStats)

	r := httptest.NewRequest("GET", "/stats/test_suite", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusOK, w.Result().StatusCode)
	suite.Require().JSONEq(`"this_is_stats"`, w.Body.String())
}

func (suite *managerTestSute) TestHTTPStatsNotFound() {
	hndl := chi.NewMux()
	hndl.Route("/stats", suite.manager.RouteStats)

	r := httptest.NewRequest("GET", "/stats/test_suite", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusNotFound, w.Result().StatusCode)
}

func (suite *managerTestSute) TestHTTPStatsWithSlashes() {
	suite.manager.Add("test:suite/with/slashes", suite)
	suite.stats = "this_is_stats"

	hndl := chi.NewMux()
	hndl.Route("/stats", suite.manager.RouteStats)

	r := httptest.NewRequest("GET", "/stats/test:suite/with/slashes", nil)
	w := httptest.NewRecorder()
	hndl.ServeHTTP(w, r)

	suite.Require().EqualValues(http.StatusOK, w.Result().StatusCode)
	suite.Require().JSONEq(`"this_is_stats"`, w.Body.String())
}

func (suite *managerTestSute) TestShutdown() {
	suite.manager.Add("test_suite", suite)

	hndl := chi.NewMux()
	hndl.Route("/ping", suite.manager.RouteHealthCheck)
	hndl.Route("/stats", suite.manager.RouteStats)
	hndl.Route("/juggler", suite.manager.RouteJugglerCheck)

	called := false
	suite.close = func() error {
		called = true
		return nil
	}

	err := suite.manager.Shutdown()
	suite.Require().NoError(err)
	suite.True(called)

	rh := httptest.NewRequest("GET", "/ping", nil)
	wh := httptest.NewRecorder()
	hndl.ServeHTTP(wh, rh)

	rs := httptest.NewRequest("GET", "/stats/test_suite", nil)
	ws := httptest.NewRecorder()
	hndl.ServeHTTP(ws, rs)

	rj := httptest.NewRequest("GET", "/juggler", nil)
	wj := httptest.NewRecorder()
	hndl.ServeHTTP(wj, rj)

	suite.Equal(http.StatusServiceUnavailable, wh.Result().StatusCode)
	suite.Equal(http.StatusServiceUnavailable, ws.Result().StatusCode)
	suite.Equal(http.StatusServiceUnavailable, wj.Result().StatusCode)
}

func (suite *managerTestSute) TestShutdownError() {
	suite.manager.Add("test_suite", suite)

	called := false
	suite.close = func() error {
		called = true
		return errors.New("shutdown error")
	}

	err := suite.manager.Shutdown()
	suite.Require().Error(err)
	suite.True(called)
}

func (suite *managerTestSute) HealthCheck(context.Context) error {
	return suite.healthCheckErr
}

func (suite *managerTestSute) Close() error {
	if suite.close == nil {
		return nil
	}
	return suite.close()
}

func (suite *managerTestSute) GetStats() interface{} {
	return suite.stats
}

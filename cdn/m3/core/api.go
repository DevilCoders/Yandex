package core

import (
	"encoding/json"
	"fmt"
	"os"
	"sync"
	"time"

	"github.com/gin-gonic/gin"
	"golang.org/x/sys/unix"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/utils"
)

type Api struct {
	g *config.CmdGlobal

	// backreference on core object
	// (could be nil)
	core *Core

	t0 time.Time
}

// Creating API instance with all storage connections
func CreateApi(g *config.CmdGlobal) (*Api, error) {
	var api Api

	api.g = g

	return &api, nil
}

const (
	API_UNIXSOCKET = 0
	API_HTTPS      = 1
)

// RunHTTPServer provide run http in tcp or unix socket mode
func (a *Api) RunHTTPServer(mode int) error {
	id := "(http) (server)"
	var err error

	r := a.routerEngine()
	if mode == API_UNIXSOCKET {

		socket := a.g.Opts.M3.Socket

		if utils.Exists(socket) {
			err = os.Remove(socket)
			if err != nil {
				a.g.Log.Error(fmt.Sprintf("%s %s error removing socket file:'%s', err:'%s'",
					utils.GetGPid(), id, socket, err))
				return err
			}
		}

		a.g.Log.Debug(fmt.Sprintf("%s %s run http server on socket:'%s'",
			utils.GetGPid(), id, socket))

		// http web server needs write permissions
		// on unix socket to write
		unix.Umask(0000)

		r.RunUnix(socket)

		return err
	}
	if mode == API_HTTPS {
		var err error

		// Generating certificate and a key, we use self-signed
		// certificates and custom header http authorization with
		// shared secret
		certfile := "/tmp/dns2-api.crt"
		keyfile := "/tmp/dns2-api.key"
		port := a.g.Opts.M3.Port

		if port > 0 {

			a.g.Log.Debug(fmt.Sprintf("%s %s run http server on port:'%d'",
				utils.GetGPid(), id, port))

			if err = utils.GenCert(certfile, keyfile, false, false); err != nil {
				a.g.Log.Debug(fmt.Sprintf("%s error creating a pair key+cert, for https, err:'%s'",
					utils.GetGPid(), err))
				return err
			}
			r.RunTLS(fmt.Sprintf(":%d", port), certfile, keyfile)
		}
	}

	return err
}

// Main entry point for http requests
func (a *Api) routerEngine() *gin.Engine {

	gin.SetMode(gin.ReleaseMode)
	r := gin.New()

	r.Use(gin.Recovery())
	r.Use(a.apiRequestLogger)

	// Monitoring methods: getting paths and
	// networks objects
	r.GET("/api/v3.1/bgp/paths", a.getBgpPaths)

	r.GET("/api/v3.1/network/objects", a.getNetworkObjects)

	r.GET("/api/v3.1/server/logrotate", a.serverLogrotate)

	// Events api integration
	a.g.Events.Api(r)

	// Monitor api integration
	a.g.Monitor.Api(r)

	return r
}

func (a *Api) apiRequestLogger(c *gin.Context) {
	path, ip := a.apiRequestString(c)
	a.g.Log.Info(fmt.Sprintf("%s m3: %s '%s %s' %d %s",
		utils.GetGPid(), ip, c.Request.Method, path,
		c.Writer.Status(), c.Request.UserAgent()))
	c.Next()
}

func (a *Api) apiRequestString(c *gin.Context) (string, string) {
	ip := c.ClientIP()
	if len(ip) == 0 {
		ip = "socket"
	}

	path := c.Request.URL.Path
	raw := c.Request.URL.RawQuery
	if raw != "" {
		path = path + "?" + raw
	}

	return path, ip
}

func (a *Api) Apiloop(wg *sync.WaitGroup, mode int) {

	defer wg.Done()

	a.g.Log.Info(fmt.Sprintf("% startng a web server on unix socket: '%s'",
		utils.GetGPid(), a.g.Opts.M3.Socket))

	a.RunHTTPServer(mode)
	return
}

// ResponseInfo contains a code and message returned by the API as errors or
// informational messages inside the response.
type ResponseInfo struct {
	Code    int    `json:"code"`
	Message string `json:"message"`
}

// Response is a template.  There will also be a result struct.  There will be a
// unique response type for each response, which will include this type.
type Response struct {
	Success  bool           `json:"success"`
	Errors   []ResponseInfo `json:"errors,omitempty"`
	Messages []ResponseInfo `json:"messages,omitempty"`
}

func (a *Api) apiSendError(c *gin.Context, code int, errStr string) {
	var r Response

	a.g.Log.Error(errStr)

	r.Success = false
	var ri ResponseInfo
	ri.Code = code
	ri.Message = errStr
	r.Errors = append(r.Errors, ri)

	response, _ := json.Marshal(r)

	a.g.Log.Error(string(response))

	c.Header("Content-Type", "application/json; charset=utf-8")
	c.String(code, string(response))
}

func (a *Api) apiSendOK(c *gin.Context, code int, msgStr string) {
	var r Response

	r.Success = true
	var ri ResponseInfo
	ri.Code = code
	ri.Message = msgStr
	r.Messages = append(r.Messages, ri)

	responseBody, _ := json.Marshal(r)

	c.Header("Content-Type", "application/json; charset=utf-8")
	c.String(code, string(responseBody))
}

const (
	// getting map from bgp
	// internal caches via list
	// path
	RIB_BGP = 101

	// getting map from shared
	// map as scan, we assume
	// that MAP is updated as
	// bgp messages arrived (deep todo)
	// see update WITHDRAW section
	RIB_MAP = 102
)

func (a *Api) getBgpPaths(c *gin.Context) {
	if a.core == nil {
		a.apiSendError(c, 502, "Internal error")
		return
	}

	var err error

	// Getting paths from bgp RIB
	var paths []Path
	mode := RIB_BGP

	id := "(api)"
	switch mode {
	case RIB_BGP:
		if paths, err = a.core.sourceGetPaths(); err != nil {
			a.g.Log.Error(fmt.Sprintf("%s %s error getting paths, err:'%s'",
				utils.GetGPid(), id, err))
			a.apiSendError(c, 502, fmt.Sprintf("%s", err))
			return
		}
	case RIB_MAP:
		paths = a.core.paths.Get()
	}

	content, _ := json.Marshal(paths)

	a.apiSendOK(c, 200, string(content))
}

func (a *Api) getNetworkObjects(c *gin.Context) {
	if a.core == nil {
		a.apiSendError(c, 502, "Internal error")
		return
	}

	id := "(api)"
	var err error

	// Detecting all containers of specified types
	// in case of containers mode
	var ObjectsSources map[string][]*TObjectsSource
	if ObjectsSources, err = a.core.networkGetSources(); err != nil {
		a.g.Log.Error(fmt.Sprintf("%s %s error getting object sources, err:'%s'",
			utils.GetGPid(), id, err))
		a.apiSendError(c, 502, fmt.Sprintf("%s", err))
		return
	}

	for _, ObjectsSources := range ObjectsSources {

		for _, source := range ObjectsSources {

			var objects TObjects
			if objects, source, err = a.core.networkGetObjects(source); err != nil {

				// in container mode, if no container is created, we
				// detect this case and return 404
				mode := a.g.Opts.Runtime.NetworkMode
				if source == nil && mode == config.NETWORK_MODE_CONTAINER {
					a.g.Log.Error(fmt.Sprintf("%s %s error getting network objects, err:'%s'",
						utils.GetGPid(), id, err))
					a.apiSendError(c, 404, fmt.Sprintf("%s", err))
					return
				}

				a.g.Log.Error(fmt.Sprintf("%s %s error getting network objects, err:'%s'",
					utils.GetGPid(), id, err))
				a.apiSendError(c, 502, fmt.Sprintf("%s", err))
				return
			}

			a.g.Log.Debug(fmt.Sprintf("%s %s objects %s",
				utils.GetGPid(), id, objects.AsString()))
			content, _ := json.Marshal(objects)
			a.apiSendOK(c, 200, string(content))
			return
		}
	}

	a.apiSendError(c, 404, fmt.Sprintf("%s", err))
}

func (a *Api) serverLogrotate(c *gin.Context) {
	if a.core == nil {
		a.apiSendError(c, 502, "Internal error")
		return
	}

	id := "(api)"
	operation := c.Query("operation")

	var err error
	var LogrotateOptions config.LogrotateOptions
	LogrotateOptions.Move = false
	if operation == "move" {
		LogrotateOptions.Move = true
	}

	if err = a.g.Log.RotateLog(&LogrotateOptions); err != nil {
		a.g.Log.Error(fmt.Sprintf("%s %s error logrotating, err:'%s'",
			utils.GetGPid(), id, err))
		a.apiSendError(c, 502, fmt.Sprintf("%s", err))
		return
	}

	a.apiSendOK(c, 200, "OK")
}

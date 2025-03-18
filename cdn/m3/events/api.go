package events

import (
	"encoding/json"
	"errors"
	"fmt"
	"net/http"

	"github.com/gin-gonic/gin"

	"a.yandex-team.ru/cdn/m3/utils"
)

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

func (e *Events) Api(r *gin.Engine) error {
	e.gin = r

	// Gettting event, :class could be "error"
	r.GET("/api/v3.0/events/:class", e.ApiGetEvents)
	r.GET("/api/v3.0/events", e.ApiGetEvents)

	// Garbage collecting, :class could be "force", "normal"
	r.PUT("/api/v3.0/gc", e.ApiGC)
	r.PUT("/api/v3.0/gc/:class", e.ApiGC)

	return nil
}

// Error function (simple variant)
func (e *Events) apiSendError(c *gin.Context, code int, errStr string) {
	var r Response
	e.log.Error(errStr)

	r.Success = false
	var ri ResponseInfo
	ri.Code = code
	ri.Message = errStr
	r.Errors = append(r.Errors, ri)

	responseBody, _ := json.Marshal(r)

	c.Header("Content-Type", "application/json; charset=utf-8")
	c.String(code, string(responseBody))
}

// OK function
func (e *Events) apiSendOK(c *gin.Context, code int, msgStr string) {
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

// OK function, raw data
func (e *Events) apiRawSendOK(c *gin.Context, code int, content string) {
	c.Header("Content-Type", "application/json; charset=utf-8")
	c.String(code, content)
}

// ApiGetEvents: server handler
func (e *Events) ApiGetEvents(c *gin.Context) {
	var err error
	var content string
	class := c.Param("class")

	e.log.Debug(fmt.Sprintf("%s api/events: get events call recevied for class:'%s'",
		utils.GetGPid(), class))

	classes := []string{"error"}

	if !utils.StringInSlice(class, classes) {
		e.apiSendError(c, 400, fmt.Sprintf("class is not expected"))
		return
	}

	if class == "error" {
		var events []ErrorEvent
		if events, err = e.GetErrorEvents(class); err != nil {
			// Failed to get events
			errExternal := fmt.Sprintf("Error getting events")
			e.apiSendError(c, 502, fmt.Sprintf("%s, err:'%s'",
				errExternal, err))
			return
		}

		e.log.Debug(fmt.Sprintf("%s api/events: shared memory returned error events:'%d'",
			utils.GetGPid(), len(events)))

		if content, err = utils.ConvMarshal(events, "JSON"); err != nil {
			errExternal := fmt.Sprintf("Error unmarshalling events")
			e.apiSendError(c, 502, fmt.Sprintf("%s, err:'%s'",
				errExternal, err))
			return
		}

		if len(events) == 0 {
			e.apiSendError(c, 404, fmt.Sprintf("Not found"))
			return
		}
	}

	e.apiRawSendOK(c, 200, string(content))
}

// ApiGC: server handler
func (e *Events) ApiGC(c *gin.Context) {
	var err error
	class := c.Param("class")
	if len(class) == 0 {
		class = "normal"
	}
	classes := []string{"force", "normal"}

	if !utils.StringInSlice(class, classes) {
		e.apiSendError(c, 400, fmt.Sprintf("class is not expected"))
		return
	}

	if err = e.GC(class); err != nil {
		e.log.Error(fmt.Sprintf("%s api/gc, error:'%s'",
			utils.GetGPid(), err))
		errExternal := fmt.Sprintf("Error garbage collector")
		e.apiSendError(c, 502, fmt.Sprintf("%s, err:'%s'",
			errExternal, err))
		return
	}

	e.apiRawSendOK(c, 200, fmt.Sprintf("garbage collected: OK"))
}

// apiClientGetEvents: client call
func (e *Events) apiClientGetEvents(class string) ([]ErrorEvent, int, error) {
	var content []byte
	var err error
	var code int

	url := "api/v3.0/events"
	if len(class) > 0 {
		url = fmt.Sprintf("api/v3.0/events/%s", class)
	}

	e.log.Debug(fmt.Sprintf("%s api/client call url:'%s', via:'%s'",
		utils.GetGPid(), url, e.socket))

	if content, err, code = utils.HttpUnixRequest(http.MethodGet, url, e.socket, nil); err != nil {
		e.log.Debug(fmt.Sprintf("%s api/client, code:'%d', error:'%s'",
			utils.GetGPid(), code, err))
		return nil, code, err
	}

	e.log.Debug(fmt.Sprintf("%s api/call returned '%s'",
		utils.GetGPid(), content))

	if class == "error" {
		var ee []ErrorEvent
		if err = json.Unmarshal([]byte(content), &ee); err != nil {
			return nil, 502, err
		}
		return ee, 200, nil
	}

	err = errors.New(fmt.Sprintf("error: no corresponding class found"))
	return nil, 400, err
}

func (e *Events) apiClientGC(force bool) error {
	var content []byte
	var err error
	var code int

	url := "api/v3.0/gc"
	if force {
		url = "api/v3.0/gc/force"
	}
	e.log.Debug(fmt.Sprintf("%s api/client call url:'%s', via:'%s'",
		utils.GetGPid(), url, e.socket))

	if content, err, code = utils.HttpUnixRequest(http.MethodPut, url, e.socket, nil); err != nil {
		e.log.Error(fmt.Sprintf("%s api/client, code:'%d', error:'%s'",
			utils.GetGPid(), code, err))
		return err
	}

	e.log.Debug(fmt.Sprintf("%s api/call returned '%s'",
		utils.GetGPid(), content))

	return nil
}

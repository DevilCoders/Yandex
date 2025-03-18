package monitor

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

func (m *Monitor) Api(r *gin.Engine) error {
	m.gin = r

	// Gettting monitor checks, could be one or all
	// registered before
	r.GET("/api/v3.0/monitor/:id", m.ApiGetMonitorCheck)
	r.GET("/api/v3.0/monitor", m.ApiGetMonitorCheck)

	return nil
}

// Error function (simple variant)
func (m *Monitor) apiSendError(c *gin.Context, code int, errStr string) {
	var r Response
	m.log.Error(errStr)

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
func (m *Monitor) apiSendOK(c *gin.Context, code int, msgStr string) {
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
func (m *Monitor) apiRawSendOK(c *gin.Context, code int, content string) {
	c.Header("Content-Type", "application/json; charset=utf-8")
	c.String(code, content)
}

// ApiGetEvents: server handler
func (m *Monitor) ApiGetMonitorCheck(c *gin.Context) {
	var err error
	var content string
	var check Check

	id := c.Param("id")
	m.log.Debug(fmt.Sprintf("%s api/monitor: reqest to get check for:'%s'",
		utils.GetGPid(), id))

	if check, err = m.GetLastHistoryCheck(id); err != nil {
		m.apiSendError(c, 404, fmt.Sprintf("%s", err))
		return
	}

	// Generating output as an array
	var out []Check
	out = append(out, check)

	if content, err = utils.ConvMarshal(out, "JSON"); err != nil {
		errExternal := fmt.Sprintf("Error marshalling monitoring checks")
		m.apiSendError(c, 502, fmt.Sprintf("%s, err:'%s'",
			errExternal, err))
		return
	}

	if len(content) == 0 {
		m.apiSendError(c, 404, fmt.Sprintf("Not found"))
		return
	}

	m.log.Debug(fmt.Sprintf("%s api/monitor: responsed with count:'%d', response length:'%d'",
		utils.GetGPid(), len(out), len(content)))

	m.apiRawSendOK(c, 200, string(content))
}

// apiClientGetEvents: client call
func (m *Monitor) apiClientGetChecks(id string) ([]Check, int, error) {
	var content []byte
	var err error
	var code int

	url := "api/v3.0/monitor"
	if len(id) > 0 {
		url = fmt.Sprintf("api/v3.0/monitor/%s", id)
	}

	m.log.Debug(fmt.Sprintf("%s api/client call url:'%s', via:'%s'",
		utils.GetGPid(), url, m.socket))

	if content, err, code = utils.HttpUnixRequest(http.MethodGet, url, m.socket, nil); err != nil {
		m.log.Debug(fmt.Sprintf("%s api/client, code:'%d', error:'%s'",
			utils.GetGPid(), code, err))
		return nil, code, err
	}

	m.log.Debug(fmt.Sprintf("%s api/call returned '%s'",
		utils.GetGPid(), content))

	var c []Check
	if err = json.Unmarshal([]byte(content), &c); err != nil {
		return nil, 502, err
	}

	m.log.Debug(fmt.Sprintf("%s checks returned, count:'%d'",
		utils.GetGPid(), len(c)))

	// Here we assume that an array contains not more than 1 response
	if len(c) == 0 {
		err = errors.New(fmt.Sprintf("error: no corresponding id:'%s' found", id))
		return nil, 400, err
	}
	return c, code, nil
}

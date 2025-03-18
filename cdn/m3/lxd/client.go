package lxd

import (
	"bytes"
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
	"strings"
	"time"

	lxd "github.com/lxc/lxd/client"
	"github.com/lxc/lxd/shared/api"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/utils"
)

// Client is a facade to thin the interface to map
// the cri logic to lxd
type Client struct {
	server lxd.InstanceServer

	g *config.CmdGlobal
}

type Container struct {
	Name       string         `json:"name"`
	Type       string         `json:"type"`
	Status     string         `json:"status"`
	StatusCode api.StatusCode `json:"status-code"`
}

func (c *Container) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("name:'%s'", c.Name))
	out = append(out, fmt.Sprintf("status:'%s'", c.Status))
	out = append(out, fmt.Sprintf("status-code:'%s'",
		c.StatusCode.String()))

	return fmt.Sprintf("%s", strings.Join(out, ","))
}

// Create connection to lxd and return a lxd client
func CreateClient(g *config.CmdGlobal) (*Client, error) {
	var err error
	var c Client

	socket := g.Opts.Lxd.Socket
	args := lxd.ConnectionArgs{
		HTTPClient: &http.Client{
			Timeout: 10 * time.Second,
		},
	}
	if c.server, err = lxd.ConnectLXDUnix(socket, &args); err != nil {
		g.Log.Error(fmt.Sprintf("%s lxd server could not be connected, socket:'%s', err:'%s'",
			utils.GetGPid(), socket, err))
		return nil, err
	}

	c.g = g
	return &c, nil
}

// ListContainers returns a list of all available containers
func (l *Client) ListContainers(prefixes []string) ([]Container, error) {
	var err error
	var containers []api.Container
	if containers, err = l.server.GetContainers(); err != nil {
		return nil, err
	}

	var out []Container
	for _, c := range containers {
		var Container Container

		// Filtering containers for prefixes/types
		for _, p := range prefixes {
			if strings.HasPrefix(c.Name, fmt.Sprintf("%s-", p)) {
				Container.Name = c.Name
				Container.Type = p
			}
		}
		if Container.Name != c.Name {
			continue
		}

		Container.Status = c.Status
		Container.StatusCode = c.StatusCode

		out = append(out, Container)
	}

	return out, err
}

// Response returns the stdout and err from an exec call
type Response struct {
	StdOut []byte
	StdErr []byte
	Code   int
}

// Exec runs a command inside container and blocks till it's finished
// returning some Response
func (l *Client) Exec(container string, cmd []string) (*Response, error) {
	var err error

	tempStderr := utils.NewWriteCloserBuffer()
	tempStdout := utils.NewWriteCloserBuffer()

	done := make(chan bool)

	id := "(exec)"
	l.g.Log.Debug(fmt.Sprintf("%s %s request container:'%s' cmd:'%s'",
		utils.GetGPid(), id, container, strings.Join(cmd, " ")))

	var operation lxd.Operation
	if operation, err = l.server.ExecContainer(container, api.ContainerExecPost{
		Command:     cmd,
		Interactive: false,
		Width:       0,
		Height:      0,
		WaitForWS:   true,
	}, &lxd.ContainerExecArgs{
		Stderr:   tempStderr,
		Stdout:   tempStdout,
		Stdin:    ioutil.NopCloser(bytes.NewReader(nil)),
		DataDone: done,
	}); err != nil {
		l.g.Log.Error(fmt.Sprintf("%s lxd exec error, command:'%s', err:'%s'",
			utils.GetGPid(), strings.Join(cmd, " "), err))
		return nil, err
	}

	if err = operation.Wait(); err != nil {
		l.g.Log.Error(fmt.Sprintf("%s lxd error wating operation,err:'%s'",
			utils.GetGPid(), err))
		return nil, err
	}

	var has bool
	var ret float64
	if ret, has = operation.Get().Metadata["return"].(float64); !has {
		err = errors.New("lxd error no returning code")
		l.g.Log.Error(fmt.Sprintf("%s error getting on metadata, err:'%s'",
			utils.GetGPid(), err))
		return nil, err
	}

	// wait till all data is written (stdout, stderr)
	<-done

	Response := &Response{
		StdErr: tempStderr.Bytes(),
		StdOut: tempStdout.Bytes(),
		Code:   int(ret),
	}

	l.g.Log.Debug(fmt.Sprintf("%s %s exec successfully returns code:'%d'",
		utils.GetGPid(), id, Response.Code))

	l.g.LogDump(fmt.Sprintf("%s stdout", utils.GetGPid()),
		string(Response.StdOut))

	l.g.LogDump(fmt.Sprintf("%s stderr", utils.GetGPid()),
		string(Response.StdErr))

	return Response, nil
}

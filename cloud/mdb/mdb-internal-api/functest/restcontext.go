package functest

import (
	"context"
	"os"

	"github.com/go-resty/resty/v2"
)

type restContext struct {
	Client    *resty.Client
	Headers   map[string]string
	NamedData map[string]string

	LastResponse *resty.Response
}

func newRESTContext() *restContext {
	rc := &restContext{Client: resty.New()}
	rc.Reset()
	return rc
}

func (rc *restContext) Reset() {
	rc.Headers = make(map[string]string)
	rc.NamedData = make(map[string]string)
}

func restURIBase() string {
	port, ok := os.LookupEnv("DBAAS_INTERNAL_API_RECIPE_PORT")
	if !ok {
		panic("dbaas internal api port is not set")
	}

	return "http://localhost:" + port
}

func pillarConfigURIBase() string {
	port, ok := os.LookupEnv("MDB_PILLAR_CONFIG_PORT")
	if !ok {
		panic("pillar config is not set")
	}

	return "http://localhost:" + port
}

type RestCall func(context.Context, string) (*resty.Response, error)

package conductor

import (
	"context"
	"fmt"
	"io/ioutil"
	"net/http"
	"strings"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
)

const (
	ConductorURL = "https://c.yandex-team.ru"
)

type ConductorClientIface interface {
	GetHostsByGroupName(ctx context.Context, groupName string) ([]string, error)
}

////////////////////////////////////////////////////////////////////////////////

type conductorClient struct {
	log nbs.Log
}

func (c *conductorClient) GetHostsByGroupName(
	ctx context.Context,
	groupName string,
) ([]string, error) {
	resp, err := http.Get(fmt.Sprintf("%v/api/groups2hosts/%v", ConductorURL, groupName))
	if err != nil {
		return nil, fmt.Errorf("GetHostsByGroupName. Request error: %w", err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, fmt.Errorf("GetHostsByGroupName. Read body error: %w", err)
	}

	result := []string{}

	for _, line := range strings.Split(string(body), "\n") {
		if line != "" {
			result = append(result, line)
		}
	}

	return result, nil
}

////////////////////////////////////////////////////////////////////////////////

func NewClient(log nbs.Log) ConductorClientIface {
	return &conductorClient{
		log: log,
	}
}

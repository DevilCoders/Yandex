package httpapi

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const conductorExecuterDataRequestTimeout = time.Minute

func (c *Client) ExecuterData(ctx context.Context, projectName string) (conductor.ExecuterData, error) {
	res := conductor.ExecuterData{}
	url := fmt.Sprintf("%s/api/executer_data/%s?format=json", URL, projectName)
	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return res, err
	}

	ctx, cancel := context.WithTimeout(ctx, conductorExecuterDataRequestTimeout)
	defer cancel()
	req = req.WithContext(ctx)

	resp, err := c.httpClient.Do(req, "Conductor Executer Data")
	if err != nil {
		return res, err
	}
	defer func() { _ = resp.Body.Close() }()

	if resp.StatusCode != http.StatusOK {
		b, _ := ioutil.ReadAll(resp.Body)
		return res, xerrors.Errorf("request failed with code %d: %s", resp.StatusCode, b)

	}

	if err = json.NewDecoder(resp.Body).Decode(&res); err != nil {
		b, _ := ioutil.ReadAll(resp.Body)
		ctxlog.Error(ctx, c.l, "failed decode response", log.Error(err), log.Binary("responseBody", b))
		return res, xerrors.Errorf("decode response: %w", err)
	}

	return res, nil
}

package http

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Client provides interface to YARN resource manager
type Client struct {
	ServerAddress string
	Logger        log.Logger
}

func doGet(ctx context.Context, url string) ([]byte, error) {
	client := &http.Client{}
	request, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return nil, xerrors.Errorf("failed to build request: %w", err)
	}
	request = request.WithContext(ctx)

	resp, err := client.Do(request)
	if err != nil {
		return nil, xerrors.Errorf("failed to get status from server %q: %w", url, err)
	}
	defer func() { _ = resp.Body.Close() }()
	if resp.StatusCode != http.StatusOK {
		return nil, xerrors.Errorf("server responded with an error: %s", resp.Status)
	}
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, xerrors.Errorf("failed to read response body: %w", err)
	}
	return body, nil
}

// SearchApplication allows to find an application by its tag
func (client *Client) SearchApplicationByTag(ctx context.Context, tag string) ([]yarn.Application, error) {
	url := client.ServerAddress + "/ws/v1/cluster/apps?applicationTags=" + tag
	body, err := doGet(ctx, url)
	if err != nil {
		return nil, err
	}

	return parseAppsResponse(body)
}

// GetApplicationAttempts allows to find an attempts and container ids by application id
func (client *Client) GetApplicationAttempts(ctx context.Context, applicationID string) ([]yarn.ApplicationAttempt, error) {
	url := client.ServerAddress + "/ws/v1/cluster/apps/" + applicationID + "/appattempts"
	body, err := doGet(ctx, url)
	if err != nil {
		return nil, err
	}

	return parseApplicationAttemptResponse(body)
}

// GetApplicationAttempts allows to find applications with and container ids
func (client *Client) SearchApplication(ctx context.Context, tag string) ([]yarn.Application, error) {
	apps, err := client.SearchApplicationByTag(ctx, tag)
	var appsWithAttempts []yarn.Application
	if err != nil {
		return nil, err
	}
	for _, app := range apps {
		applicationAttemps, err := client.GetApplicationAttempts(ctx, app.ID)
		if err != nil {
			return nil, err
		}
		for _, applicationAttempt := range applicationAttemps {
			app.ApplicationAttempts = append(
				app.ApplicationAttempts,
				yarn.ApplicationAttempt{
					AppAttemptID:  applicationAttempt.AppAttemptID,
					AMContainerID: applicationAttempt.AMContainerID,
				},
			)
		}
		appsWithAttempts = append(appsWithAttempts, app)
	}
	return appsWithAttempts, nil
}

func parseAppsResponse(body []byte) ([]yarn.Application, error) {
	var responseDocument struct {
		Apps struct {
			App []struct {
				Name        string `json:"name"`
				ID          string `json:"id"`
				FinalStatus string `json:"finalStatus"`
				State       string `json:"state"`
				StartedTime int    `json:"startedTime"`
			} `json:"app"`
		} `json:"apps"`
	}

	err := json.Unmarshal(body, &responseDocument)
	if err != nil {
		return nil, err
	}
	apps := responseDocument.Apps.App
	var yarnApps []yarn.Application
	for _, app := range apps {
		yarnApp := yarn.Application{
			Name:        app.Name,
			ID:          app.ID,
			State:       yarn.ToState(app.State),
			FinalStatus: yarn.ToFinalStatus(app.FinalStatus),
			StartedTime: app.StartedTime,
		}
		yarnApps = append(yarnApps, yarnApp)
	}
	return yarnApps, nil
}

func parseApplicationAttemptResponse(body []byte) ([]yarn.ApplicationAttempt, error) {
	var responseDocument struct {
		AppAttempts struct {
			AppAttempt []struct {
				AppAttemptID  string `json:"appAttemptId"`
				AMContainerID string `json:"containerId"`
			} `json:"appAttempt"`
		} `json:"appAttempts"`
	}

	err := json.Unmarshal(body, &responseDocument)
	if err != nil {
		return nil, err
	}
	var applicationAttempts []yarn.ApplicationAttempt
	for _, applicationAttempt := range responseDocument.AppAttempts.AppAttempt {
		applicationAttempt := yarn.ApplicationAttempt{
			AppAttemptID:  applicationAttempt.AppAttemptID,
			AMContainerID: applicationAttempt.AMContainerID,
		}
		applicationAttempts = append(applicationAttempts, applicationAttempt)
	}
	return applicationAttempts, nil
}

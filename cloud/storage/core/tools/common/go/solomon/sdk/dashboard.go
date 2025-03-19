package sdk

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
)

type DashboardID = string

type DashboardParameters struct {
	Name  string `json:"name"`
	Value string `json:"value"`
}

type DashboardPanels struct {
	Colspan  uint   `json:"colspan"`
	Markdown string `json:"markdown"`
	Rowspan  uint   `json:"rowspan"`
	Subtitle string `json:"subtitle"`
	Title    string `json:"title"`
	Type     string `json:"type"`
	URL      string `json:"url"`
}

type DashboardRows struct {
	Panels []DashboardPanels `json:"panels"`
}

type Dashboard struct {
	ID        string `json:"id"`
	ProjectID string `json:"projectId"`
	Version   uint   `json:"version"`

	Name             string                `json:"name"`
	Description      string                `json:"description"`
	HeightMultiplier float32               `json:"heightMultiplier"`
	Parameters       []DashboardParameters `json:"parameters"`
	Rows             []DashboardRows       `json:"rows"`
}

type listDashboardsItem struct {
	ID string `json:"id"`
}

type listDashboardsResponse struct {
	Items []listDashboardsItem `json:"result"`
	Page  listPage             `json:"page"`
}

func (s *solomonClient) listDashboards(
	ctx context.Context,
	pageNum uint,
) ([]DashboardID, listPage, error) {

	url := s.url + "/" + s.projectID + "/dashboards?pageSize=1000"
	url += "&page=" + fmt.Sprint(pageNum)

	body, err := s.executeRequest(ctx, "listDashboards", "GET", url, nil)
	if err != nil {
		return nil, listPage{}, err
	}

	r := &listDashboardsResponse{}
	if err := json.Unmarshal(body, r); err != nil {
		return nil, listPage{}, fmt.Errorf("listDashboards. Unmarshal error: %w", err)
	}

	var result []string

	for _, item := range r.Items {
		result = append(result, item.ID)
	}

	return result, r.Page, nil
}

func (s *solomonClient) ListDashboards(
	ctx context.Context,
) ([]DashboardID, error) {

	var result []string

	pageNum := uint(0)
	for {
		clusters, page, err := s.listDashboards(ctx, pageNum)
		if err != nil {
			return result, err
		}

		result = append(result, clusters...)
		pageNum = page.Current + 1

		if pageNum >= page.PagesCount {
			break
		}
	}

	return result, nil
}

func (s *solomonClient) GetDashboard(
	ctx context.Context,
	dashboardID DashboardID,
) (Dashboard, error) {

	url := s.url + "/" + s.projectID + "/dashboards/" + dashboardID

	body, err := s.executeRequest(ctx, "GetDashboard", "GET", url, nil)
	if err != nil {
		return Dashboard{}, err
	}

	r := &Dashboard{}
	if err := json.Unmarshal(body, r); err != nil {
		return Dashboard{}, fmt.Errorf("GetDashboard. Unmarshal error: %w", err)
	}

	return *r, nil
}

func (s *solomonClient) AddDashboard(
	ctx context.Context,
	dashboard Dashboard,
) (Dashboard, error) {

	url := s.url + "/" + s.projectID + "/dashboards"

	payload, err := json.Marshal(dashboard)
	if err != nil {
		return Dashboard{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"AddDashboard",
		"POST",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Dashboard{}, err
	}

	r := &Dashboard{}
	if err := json.Unmarshal(body, r); err != nil {
		return Dashboard{}, err
	}

	return *r, nil
}

func (s *solomonClient) UpdateDashboard(
	ctx context.Context,
	dashboard Dashboard,
) (Dashboard, error) {

	url := s.url + "/" + s.projectID + "/dashboards/" + dashboard.ID

	payload, err := json.Marshal(dashboard)
	if err != nil {
		return Dashboard{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"UpdateDashboard",
		"PUT",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Dashboard{}, err
	}

	r := &Dashboard{}
	if err := json.Unmarshal(body, r); err != nil {
		return Dashboard{}, err
	}

	return *r, nil
}

func (s *solomonClient) DeleteDashboard(
	ctx context.Context,
	dashboardID DashboardID,
) error {

	url := s.url + "/" + s.projectID + "/dashboards/" + dashboardID

	_, err := s.executeRequest(ctx, "DeleteDashboard", "DELETE", url, nil)
	if err != nil {
		return err
	}

	return nil
}

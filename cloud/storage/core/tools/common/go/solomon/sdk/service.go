package sdk

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
)

type ServiceID = string

type ServiceAggrRules struct {
	Cond     []string `json:"cond"`
	Function string   `json:"function,omitempty"`
	Target   []string `json:"target"`
}

type ServicePriorityRules struct {
	Priority int    `json:"priority"`
	Target   string `json:"target"`
}

type ServiceSensorConf struct {
	AggrRules      []ServiceAggrRules `json:"aggrRules,omitempty"`
	RawDataMemOnly bool               `json:"rawDataMemOnly"`
}

type Service struct {
	ID        string `json:"id"`
	ProjectID string `json:"projectId"`
	Name      string `json:"name"`
	Version   uint   `json:"version"`

	AddTSArgs       bool              `json:"addTsArgs"`
	GridSec         int               `json:"gridSec"`
	Interval        int               `json:"interval"`
	Path            string            `json:"path"`
	Port            int               `json:"port"`
	Protocol        string            `json:"protocol"`
	SensorConf      ServiceSensorConf `json:"sensorConf,omitempty"`
	SensorNameLabel string            `json:"sensorNameLabel"`
	SensorsTTLDays  int               `json:"sensorsTtlDays"`
	TvmDestID       string            `json:"tvmDestId"`
}

type listServicesItem struct {
	ID string `json:"id"`
}

type listServicesResponse struct {
	Items []listServicesItem `json:"result"`
	Page  listPage           `json:"page"`
}

func (s *solomonClient) listServices(
	ctx context.Context,
	pageNum uint,
) ([]ServiceID, listPage, error) {

	url := s.url + "/" + s.projectID + "/services?pageSize=1000"
	url += "&page=" + fmt.Sprint(pageNum)

	body, err := s.executeRequest(ctx, "listServices", "GET", url, nil)
	if err != nil {
		return nil, listPage{}, err
	}

	r := &listServicesResponse{}
	if err := json.Unmarshal(body, r); err != nil {
		return nil, listPage{}, fmt.Errorf("listServices. Unmarshal error: %w", err)
	}

	var result []string

	for _, item := range r.Items {
		result = append(result, item.ID)
	}

	return result, r.Page, nil
}

func (s *solomonClient) ListServices(
	ctx context.Context,
) ([]ServiceID, error) {

	var result []string

	pageNum := uint(0)
	for {
		clusters, page, err := s.listServices(ctx, pageNum)
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

func (s *solomonClient) GetService(
	ctx context.Context,
	serviceID ServiceID,
) (Service, error) {

	url := s.url + "/" + s.projectID + "/services/" + serviceID

	body, err := s.executeRequest(ctx, "GetService", "GET", url, nil)
	if err != nil {
		return Service{}, err
	}

	r := &Service{}
	if err := json.Unmarshal(body, r); err != nil {
		return Service{}, fmt.Errorf("GetService. Unmarshal error: %w", err)
	}

	return *r, nil
}

func (s *solomonClient) AddService(
	ctx context.Context,
	service Service,
) (Service, error) {

	url := s.url + "/" + s.projectID + "/services"

	payload, err := json.Marshal(service)
	if err != nil {
		return Service{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"AddService",
		"POST",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Service{}, err
	}

	r := &Service{}
	if err := json.Unmarshal(body, r); err != nil {
		return Service{}, err
	}

	return *r, nil
}

func (s *solomonClient) UpdateService(
	ctx context.Context,
	service Service,
) (Service, error) {

	url := s.url + "/" + s.projectID + "/services/" + service.ID

	payload, err := json.Marshal(service)
	if err != nil {
		return Service{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"UpdateService",
		"PUT",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Service{}, err
	}

	r := &Service{}
	if err := json.Unmarshal(body, r); err != nil {
		return Service{}, err
	}

	return *r, nil
}

func (s *solomonClient) DeleteService(
	ctx context.Context,
	serviceID ServiceID,
) error {

	url := s.url + "/" + s.projectID + "/services/" + serviceID

	_, err := s.executeRequest(ctx, "DeleteService", "DELETE", url, nil)
	if err != nil {
		return err
	}

	return nil
}

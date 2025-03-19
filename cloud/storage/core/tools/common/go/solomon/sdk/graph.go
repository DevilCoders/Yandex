package sdk

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
)

type GraphID = string

type GraphParameters struct {
	Name  string `json:"name"`
	Value string `json:"value"`
}

type GraphSelectors struct {
	Name  string `json:"name"`
	Value string `json:"value"`
}

type GraphElements struct {
	Title     string `json:"title"`
	Type      string `json:"type"`
	Stack     string `json:"stack"`
	Color     string `json:"color"`
	Link      string `json:"link"`
	Transform string `json:"transform"`
	Yaxis     string `json:"yaxis"`

	Selectors []GraphSelectors `json:"selectors"`
}

type Graph struct {
	ID        string `json:"id"`
	ProjectID string `json:"projectId"`
	Version   uint   `json:"version"`

	Name        string `json:"name"`
	Description string `json:"description"`

	Parameters []GraphParameters `json:"parameters"`
	Elements   []GraphElements   `json:"elements"`

	Aggr                string `json:"aggr"`
	BucketLabel         string `json:"bucketLabel"`
	ColorScheme         string `json:"colorScheme"`
	Downsampling        string `json:"downsampling"`
	DownsamplingAggr    string `json:"downsamplingAggr"`
	DownsamplingFill    string `json:"downsamplingFill"`
	Filter              string `json:"filter"`
	FilterBy            string `json:"filterBy"`
	FilterLimit         string `json:"filterLimit"`
	GraphMode           string `json:"graphMode"`
	Green               string `json:"green"`
	GreenValue          string `json:"greenValue"`
	Grid                string `json:"grid"`
	HideNoData          bool   `json:"hideNoData"`
	IgnoreInf           bool   `json:"ignoreInf"`
	IgnoreMinStepMillis bool   `json:"ignoreMinStepMillis"`
	Interpolate         string `json:"interpolate"`
	Limit               string `json:"limit"`
	Max                 string `json:"max"`
	MaxPoints           int    `json:"maxPoints"`
	Min                 string `json:"min"`
	MovingPercentile    string `json:"movingPercentile"`
	MovingWindow        string `json:"movingWindow"`
	Normalize           bool   `json:"normalize"`
	NumberFormat        string `json:"numberFormat"`
	OverLinesTransform  string `json:"overLinesTransform"`
	Percentiles         string `json:"percentiles"`
	Red                 string `json:"red"`
	RedValue            string `json:"redValue"`
	Scale               string `json:"scale"`
	SecondaryGraphMode  string `json:"secondaryGraphMode"`
	Transform           string `json:"transform"`
	Violet              string `json:"violet"`
	VioletValue         string `json:"violetValue"`
	Yellow              string `json:"yellow"`
	YellowValue         string `json:"yellowValue"`
}

type listGraphsItem struct {
	ID string `json:"id"`
}

type listGraphsResponse struct {
	Items []listGraphsItem `json:"result"`
	Page  listPage         `json:"page"`
}

func (s *solomonClient) listGraphs(
	ctx context.Context,
	pageNum uint,
) ([]GraphID, listPage, error) {

	url := s.url + "/" + s.projectID + "/graphs?pageSize=1000"
	url += "&page=" + fmt.Sprint(pageNum)

	body, err := s.executeRequest(ctx, "listGraphs", "GET", url, nil)
	if err != nil {
		return nil, listPage{}, err
	}

	r := &listGraphsResponse{}
	if err := json.Unmarshal(body, r); err != nil {
		return nil, listPage{}, fmt.Errorf("listGraphs. Unmarshal error: %w", err)
	}

	var result []string

	for _, item := range r.Items {
		result = append(result, item.ID)
	}

	return result, r.Page, nil
}

func (s *solomonClient) ListGraphs(
	ctx context.Context,
) ([]GraphID, error) {

	var result []string

	pageNum := uint(0)
	for {
		clusters, page, err := s.listGraphs(ctx, pageNum)
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

func (s *solomonClient) GetGraph(
	ctx context.Context,
	graphID GraphID,
) (Graph, error) {

	url := s.url + "/" + s.projectID + "/graphs/" + graphID

	body, err := s.executeRequest(ctx, "GetGraph", "GET", url, nil)
	if err != nil {
		return Graph{}, err
	}

	r := &Graph{}
	if err := json.Unmarshal(body, r); err != nil {
		return Graph{}, fmt.Errorf("GetGraph. Unmarshal error: %w", err)
	}

	return *r, nil
}

func (s *solomonClient) AddGraph(
	ctx context.Context,
	graph Graph,
) (Graph, error) {

	url := s.url + "/" + s.projectID + "/graphs"

	payload, err := json.Marshal(graph)
	if err != nil {
		return Graph{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"AddGraph",
		"POST",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Graph{}, err
	}

	r := &Graph{}
	if err := json.Unmarshal(body, r); err != nil {
		return Graph{}, err
	}

	return *r, nil
}

func (s *solomonClient) UpdateGraph(
	ctx context.Context,
	graph Graph,
) (Graph, error) {

	url := s.url + "/" + s.projectID + "/graphs/" + graph.ID

	payload, err := json.Marshal(graph)
	if err != nil {
		return Graph{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"UpdateGraph",
		"PUT",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Graph{}, err
	}

	r := &Graph{}
	if err := json.Unmarshal(body, r); err != nil {
		return Graph{}, err
	}

	return *r, nil
}

func (s *solomonClient) DeleteGraph(
	ctx context.Context,
	graphID GraphID,
) error {

	url := s.url + "/" + s.projectID + "/graphs/" + graphID

	_, err := s.executeRequest(ctx, "DeleteGraph", "DELETE", url, nil)
	if err != nil {
		return err
	}

	return nil
}

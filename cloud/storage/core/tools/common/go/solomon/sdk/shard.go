package sdk

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
)

type ShardID = string

type Shard struct {
	ID        string `json:"id"`
	ProjectID string `json:"projectId"`
	Version   uint   `json:"version"`

	ClusterID       string `json:"clusterId"`
	ClusterName     string `json:"clusterName"`
	DecimPolicy     string `json:"decimPolicy"`
	SensorNameLabel string `json:"sensorNameLabel"`
	SensorsTTLDays  int    `json:"sensorsTtlDays"`
	ServiceID       string `json:"serviceId"`
	ServiceName     string `json:"serviceName"`
	State           string `json:"state"`
}

type listShardsItem struct {
	ID string `json:"id"`
}

type listShardsResponse struct {
	Items []listShardsItem `json:"result"`
	Page  listPage         `json:"page"`
}

func (s *solomonClient) listShards(
	ctx context.Context,
	pageNum uint,
) ([]ShardID, listPage, error) {

	url := s.url + "/" + s.projectID + "/shards?pageSize=1000"
	url += "&page=" + fmt.Sprint(pageNum)

	body, err := s.executeRequest(ctx, "listShards", "GET", url, nil)
	if err != nil {
		return nil, listPage{}, err
	}

	r := &listShardsResponse{}
	if err := json.Unmarshal(body, r); err != nil {
		return nil, listPage{}, fmt.Errorf("listShards. Unmarshal error: %w", err)
	}

	var result []string

	for _, item := range r.Items {
		result = append(result, item.ID)
	}

	return result, r.Page, nil
}

func (s *solomonClient) ListShards(
	ctx context.Context,
) ([]ShardID, error) {

	var result []string

	pageNum := uint(0)
	for {
		clusters, page, err := s.listShards(ctx, pageNum)
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

func (s *solomonClient) GetShard(
	ctx context.Context,
	shardID ShardID,
) (Shard, error) {

	url := s.url + "/" + s.projectID + "/shards/" + shardID

	body, err := s.executeRequest(ctx, "GetShard", "GET", url, nil)
	if err != nil {
		return Shard{}, err
	}

	r := &Shard{}
	if err := json.Unmarshal(body, r); err != nil {
		return Shard{}, fmt.Errorf("GetShard. Unmarshal error: %w", err)
	}

	return *r, nil
}

func (s *solomonClient) AddShard(
	ctx context.Context,
	shard Shard,
) (Shard, error) {

	url := s.url + "/" + s.projectID + "/shards"

	payload, err := json.Marshal(shard)
	if err != nil {
		return Shard{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"AddShard",
		"POST",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Shard{}, err
	}

	r := &Shard{}
	if err := json.Unmarshal(body, r); err != nil {
		return Shard{}, err
	}

	return *r, nil
}

func (s *solomonClient) UpdateShard(
	ctx context.Context,
	shard Shard,
) (Shard, error) {

	url := s.url + "/" + s.projectID + "/shards/" + shard.ID

	payload, err := json.Marshal(shard)
	if err != nil {
		return Shard{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"UpdateShard",
		"PUT",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Shard{}, err
	}

	r := &Shard{}
	if err := json.Unmarshal(body, r); err != nil {
		return Shard{}, err
	}

	return *r, nil
}

func (s *solomonClient) DeleteShard(
	ctx context.Context,
	shardID ShardID,
) error {

	url := s.url + "/" + s.projectID + "/shards/" + shardID

	_, err := s.executeRequest(ctx, "DeleteShard", "DELETE", url, nil)
	if err != nil {
		return err
	}

	return nil
}

package sdk

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
)

type ChannelID = string

type ChannelTelegramMethod struct {
	GroupTitle     string `json:"groupTitle"`
	TextTemplate   string `json:"textTemplate"`
	SendScreenshot bool   `json:"sendScreenshot"`
}

type ChannelJugglerMethod struct {
	Host        string   `json:"host"`
	Service     string   `json:"service"`
	Instance    string   `json:"instance"`
	Description string   `json:"description"`
	Tags        []string `json:"tags"`
}

type ChannelMethod struct {
	Telegram *ChannelTelegramMethod `json:"telegram,omitempty"`
	Juggler  *ChannelJugglerMethod  `json:"juggler,omitempty"`
}

type Channel struct {
	ID        string        `json:"id"`
	ProjectID string        `json:"projectId"`
	Name      string        `json:"name"`
	Version   uint          `json:"version"`
	Method    ChannelMethod `json:"method"`
}

type listChannelsItem struct {
	ID string `json:"id"`
}

type listChannelsResponse struct {
	Items         []listChannelsItem `json:"items"`
	NextPageToken string             `json:"nextPageToken"`
}

func (s *solomonClient) listChannels(
	ctx context.Context,
	pageToken string,
) ([]ChannelID, string, error) {

	url := s.url + "/" + s.projectID + "/notificationChannels?pageSize=1000"
	if pageToken != "" {
		url += "&pageToken=" + pageToken
	}

	body, err := s.executeRequest(ctx, "listChannels", "GET", url, nil)
	if err != nil {
		return nil, "", err
	}

	r := &listChannelsResponse{}
	if err := json.Unmarshal(body, r); err != nil {
		return nil, "", fmt.Errorf("listChannels. Unmarshal error: %w", err)
	}

	var result []string

	for _, item := range r.Items {
		result = append(result, item.ID)
	}

	return result, r.NextPageToken, nil
}

func (s *solomonClient) ListChannels(
	ctx context.Context,
) ([]ChannelID, error) {

	var result []string

	pageToken := ""
	for {
		channels, nextPageToken, err := s.listChannels(ctx, pageToken)
		if err != nil {
			return result, err
		}

		result = append(result, channels...)
		pageToken = nextPageToken

		if pageToken == "" {
			break
		}
	}

	return result, nil
}

func (s *solomonClient) GetChannel(
	ctx context.Context,
	channelID ChannelID,
) (Channel, error) {

	url := s.url + "/" + s.projectID + "/notificationChannels/" + channelID

	body, err := s.executeRequest(ctx, "GetChannel", "GET", url, nil)
	if err != nil {
		return Channel{}, err
	}

	r := &Channel{}
	if err := json.Unmarshal(body, r); err != nil {
		return Channel{}, fmt.Errorf("GetChannel. Unmarshal error: %w", err)
	}

	return *r, nil
}

func (s *solomonClient) AddChannel(
	ctx context.Context,
	channel Channel,
) (Channel, error) {

	url := s.url + "/" + s.projectID + "/notificationChannels"

	payload, err := json.Marshal(channel)
	if err != nil {
		return Channel{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"AddChannel",
		"POST",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Channel{}, err
	}

	r := &Channel{}
	if err := json.Unmarshal(body, r); err != nil {
		return Channel{}, err
	}

	return *r, nil
}

func (s *solomonClient) UpdateChannel(
	ctx context.Context,
	channel Channel,
) (Channel, error) {

	url := s.url + "/" + s.projectID + "/notificationChannels/" + channel.ID

	payload, err := json.Marshal(channel)
	if err != nil {
		return Channel{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"UpdateChannel",
		"PUT",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Channel{}, err
	}

	r := &Channel{}
	if err := json.Unmarshal(body, r); err != nil {
		return Channel{}, err
	}

	return *r, nil
}

func (s *solomonClient) DeleteChannel(
	ctx context.Context,
	channelID ChannelID,
) error {

	url := s.url + "/" + s.projectID + "/notificationChannels/" + channelID

	_, err := s.executeRequest(ctx, "DeleteChannel", "DELETE", url, nil)
	if err != nil {
		return err
	}

	return nil
}

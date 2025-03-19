package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"
)

type Signer struct {
	app      string
	token    string
	endpoint string
	client   *http.Client
}

func NewSigner(app, token, endpoint string) *Signer {
	return &Signer{
		app:      app,
		token:    token,
		endpoint: endpoint,
		client: &http.Client{
			Timeout: 60 * time.Second,
		},
	}
}

type SignatureRequest struct {
	Application string `json:"application"`
	Filename    string `json:"filename"`
}

type SignatureResponse struct {
	ID json.Number `json:"id"`
}

func (s *Signer) CreateSignatureRequest(filename string) (string, error) {
	var buf bytes.Buffer
	if err := json.NewEncoder(&buf).Encode(SignatureRequest{Application: s.app, Filename: filename}); err != nil {
		return "", fmt.Errorf("failed to marshal signature request data: %w", err)
	}

	req, err := http.NewRequest("POST", fmt.Sprintf("%s/api/v2/sign/", s.endpoint), &buf)
	if err != nil {
		return "", fmt.Errorf("failed to create signature request: %w", err)
	}

	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("Authorization", "OAuth "+s.token)
	res, err := s.client.Do(req)
	if err != nil {
		return "", fmt.Errorf("failed to do signature request: %w", err)
	}
	defer func() {
		checkErr("failed to close signature response body: %s", res.Body.Close())
	}()

	if err = checkHTTPError(res); err != nil {
		return "", fmt.Errorf("received signatre request http error: %w", err)
	}

	var data SignatureResponse
	if err := json.NewDecoder(res.Body).Decode(&data); err != nil {
		return "", fmt.Errorf("failed to unmarshal signature response body: %w", err)
	}

	return string(data.ID), nil
}

type UploadResponse struct {
	Success bool   `json:"success"`
	SHA1    string `json:"sha1"`
	Detail  string `json:"detail"`
}

func (s *Signer) UploadFile(id string, file io.Reader) (UploadResponse, error) {
	var data UploadResponse

	req, err := http.NewRequest("PUT", fmt.Sprintf("%s/api/v2/sign/%s/", s.endpoint, id), file)
	if err != nil {
		return data, fmt.Errorf("failed to create upload request: %w", err)
	}

	req.Header.Set("Content-Type", "application/octet-stream")
	req.Header.Set("Authorization", "OAuth "+s.token)
	res, err := s.client.Do(req)
	if err != nil {
		return data, fmt.Errorf("failed to do upload request: %w", err)
	}
	defer func() {
		checkErr("failed to close upload response body: %s", res.Body.Close())
	}()

	if err = checkHTTPError(res); err != nil {
		return data, fmt.Errorf("received upload http error: %w", err)
	}

	if err = json.NewDecoder(res.Body).Decode(&data); err != nil {
		return data, fmt.Errorf("failed to unmarshal upload response body: %w", err)
	}

	if !data.Success {
		return data, fmt.Errorf("received error from signer: %s", data.Detail)
	}

	return data, nil
}

type StatusResponse struct {
	Status string `json:"status"`
	URL    string `json:"url"`
	SHA1   string `json:"sha1"`
}

func (s *Signer) CheckSignatureStatus(id string) (StatusResponse, error) {
	var data StatusResponse

	req, err := http.NewRequest("GET", fmt.Sprintf("%s/api/v2/sign/%s/", s.endpoint, id), nil)
	if err != nil {
		return data, fmt.Errorf("failed to create status request: %w", err)
	}

	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("Authorization", "OAuth "+s.token)
	res, err := s.client.Do(req)
	if err != nil {
		return data, fmt.Errorf("failed to fetch status: %w", err)
	}
	defer func() {
		checkErr("failed to close status response body: %s", res.Body.Close())
	}()

	if err = json.NewDecoder(res.Body).Decode(&data); err != nil {
		return data, fmt.Errorf("failed to unmarshal status body: %w", err)
	}

	return data, nil
}

func (s *Signer) RequestSignedFile(url string) (io.ReadCloser, error) {
	res, err := s.client.Get(url)
	if err != nil {
		return nil, fmt.Errorf("failed to fetch signed file: %w", err)
	}

	return res.Body, nil
}

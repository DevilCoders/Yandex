package ua

import (
	"bytes"
	"encoding/json"
	"fmt"
	"net/http"
	"time"
)

func Push(data []Metric, url string) error {
	payload := map[string][]Metric{}
	payload["metrics"] = append(payload["metrics"], data...)

	json_data, err := json.Marshal(payload)
	if err != nil {
		return fmt.Errorf("Failed unmarshalling for sending: %w", err)
	}

	client := &http.Client{
		Timeout: 2 * time.Second,
	}

	req, err := http.NewRequest(http.MethodPost, url, bytes.NewBuffer(json_data))
	if err != nil {
		return fmt.Errorf("Bad http request to %s: %w", url, err)
	}
	req.Header.Set("Content-Type", "application/json")

	responce, err := client.Do(req)
	if err != nil {
		return fmt.Errorf("Failed to make a request: %w", err)
	}
	defer responce.Body.Close()

	if responce.StatusCode != 200 {
		return fmt.Errorf("Failed to send metrics, got HTTP code: " + fmt.Sprint(responce.StatusCode))
	}

	return nil
}

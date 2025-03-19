package main

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
)

// checkFatalErr is a utility wrapper for fatal application errors
func checkFatalErr(format string, err error) {
	if err != nil {
		// log.Output is used instead of log.Fatal
		// to report line number for correct caller stack frame
		_ = log.Output(2, fmt.Sprintf(format, err))
		os.Exit(1)
	}
}

func checkErr(format string, err error) {
	if err != nil {
		_ = log.Output(2, fmt.Sprintf(format, err))
	}
}

type errData struct {
	Detail string `json:"detail"`
}

// checkHTTPError checks for http errors in response,
// returns error detail if present or response body wrapped in error interface
func checkHTTPError(res *http.Response) error {
	if res.StatusCode < 400 {
		return nil
	}

	body, err := io.ReadAll(res.Body)
	if err != nil {
		return fmt.Errorf("failed to read http %s error body", http.StatusText(res.StatusCode))
	}

	var data errData
	if err := json.Unmarshal(body, &data); err != nil {
		return fmt.Errorf("received http error %s, body: %s", http.StatusText(res.StatusCode), string(body))
	}

	return fmt.Errorf("received http error %s, detail: %s", http.StatusText(res.StatusCode), data.Detail)
}

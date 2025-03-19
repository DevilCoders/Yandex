package load

import (
	"encoding/json"
	"fmt"
	"io"
	"os"
	"time"

	"github.com/go-resty/resty/v2"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	defaultSeparator = '\t'
)

type requestResult struct {
	code int
	body string
}

func (r *requestResult) str() string {
	return fmt.Sprintf("%d%c%s", r.code, defaultSeparator, r.body)
}

type RequestParams struct {
	BaseURL    string
	InFileName string

	Delay time.Duration

	AuthToken string

	HostHeader string
}

func MakeRequests(params RequestParams) error {
	file, err := os.Open(params.InFileName)
	if err != nil {
		return err
	}

	defer file.Close()

	requestsCh := make(chan *requestBody, 1)

	var group errgroup.Group

	group.Go(func() error {
		defer close(requestsCh)

		dec := json.NewDecoder(file)
		for {
			var body requestBody
			err := dec.Decode(&body)
			if xerrors.Is(err, io.EOF) {
				return nil
			}

			if err != nil {
				return err
			}

			requestsCh <- &body
		}
	})

	group.Go(func() error {
		t := time.NewTicker(params.Delay)
		defer t.Stop()

		client := resty.New().
			SetBaseURL(params.BaseURL).
			SetHeader("X-YaCloud-SubjectToken", params.AuthToken)

		if params.HostHeader == "" {
			client = client.SetHeader("Host", params.HostHeader)
		}

		for range t.C {
			r, ok := <-requestsCh
			if !ok {
				return nil
			}

			response, err := makeRequest(client, r)
			if err != nil {
				return err
			}

			fmt.Printf("%s%c%s\n", r.str(), defaultSeparator, response.str())
		}

		return nil
	})

	return group.Wait()
}

func makeRequest(client *resty.Client, r *requestBody) (*requestResult, error) {
	result, err := client.R().
		SetBody(r).
		Post("")

	if err != nil {
		return nil, err
	}

	return &requestResult{
		code: result.StatusCode(),
		body: result.String(),
	}, nil
}

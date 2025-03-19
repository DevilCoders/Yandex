package startrack

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"strings"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
)

const (
	StarTrackURL = "https://st-api.yandex-team.ru"
)

type StarTrackClientIface interface {
	Comment(ctx context.Context, text string) error
}

////////////////////////////////////////////////////////////////////////////////

type starTrackClient struct {
	logutil.WithLog
	http *http.Client

	url        string
	oauthToken string

	replacer *strings.Replacer
}

func (st *starTrackClient) Comment(
	ctx context.Context,
	text string,
) error {
	type Message struct {
		Text string `json:"text"`
	}

	if st.replacer != nil {
		text = st.replacer.Replace(text)
	}

	payload, _ := json.Marshal(&Message{text})
	req, err := http.NewRequest(
		"POST",
		st.url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return fmt.Errorf("Comment. Can't create request: %w", err)
	}

	req.Header.Add("Authorization", "OAuth "+st.oauthToken)
	req.Header.Add("Content-Type", "application/json;charset=UTF-8")
	req.Header.Add("Accept", "application/json")

	resp, err := st.http.Do(req)
	if err != nil {
		return fmt.Errorf("Comment. Request error: %w", err)
	}

	defer resp.Body.Close()

	st.LogDbg(ctx, "[ST] response: %v", resp)

	if resp.StatusCode != http.StatusCreated {
		return fmt.Errorf("Comment. Bad response: %v", resp)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func NewClient(
	log nbs.Log,
	ticket string,
	oauthToken string,
	replacer *strings.Replacer,
) StarTrackClientIface {
	return &starTrackClient{
		WithLog:    logutil.WithLog{Log: log},
		http:       &http.Client{},
		url:        fmt.Sprintf("%v/v2/issues/%v/comments", StarTrackURL, ticket),
		oauthToken: oauthToken,
		replacer:   replacer,
	}
}

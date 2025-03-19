package messenger

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"strings"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
)

const (
	QMssngrURL = "https://bp.mssngr.yandex.net/bot/telegram_lite/sendMessage"
	QMssngrID  = "q"
)

////////////////////////////////////////////////////////////////////////////////

type qmssngrClient struct {
	logutil.WithLog
	http *http.Client

	url   string
	token string

	replacer *strings.Replacer

	users    string
	userFile string
}

func (q *qmssngrClient) SendMessage(
	ctx context.Context,
	chatID string,
	text string,
) error {
	type Message struct {
		Text   string `json:"text"`
		ChatID string `json:"chat_id"`
	}

	if q.replacer != nil {
		text = q.replacer.Replace(text)
	}

	payload, _ := json.Marshal(&Message{text, chatID})

	req, err := http.NewRequest("POST", q.url, bytes.NewBuffer(payload))
	if err != nil {
		return fmt.Errorf("SendMessage. Can't create request: %w", err)
	}

	req.Header.Add("Authorization", "OAuthTeam "+q.token)
	req.Header.Add("Content-Type", "application/json;charset=UTF-8")
	req.Header.Add("Accept", "application/json")

	resp, err := q.http.Do(req)
	if err != nil {
		return fmt.Errorf("SendMessage. Request error: %w", err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return fmt.Errorf("SendMessage. Read body error: %w", err)
	}

	result := &struct {
		Failstr string `json:"error"`
	}{}

	if err := json.Unmarshal(body, result); err != nil {
		return fmt.Errorf("SendMessage. Unmarshal error: %w", err)
	}

	if len(result.Failstr) != 0 {
		q.LogDbg(ctx, "[Q] error: %v; body: %v", result.Failstr, string(body))

		return fmt.Errorf("SendMessage. Bad response json: %v", string(body))
	}

	q.LogDbg(ctx, "[Q] body: %v", string(body))

	return nil
}

func (q *qmssngrClient) GetMessengerType() MessengerType {
	return QMssngr
}

func (q *qmssngrClient) GetUserChatID(username string) string {
	return getUserChatID(username, q.users)
}

func (q *qmssngrClient) GetUsersFile() string {
	return q.userFile
}

////////////////////////////////////////////////////////////////////////////////

func NewQMssngrClient(
	log nbs.Log,
	token string,
	replacer *strings.Replacer,
	users string,
	userFile string,
) MessengerClientIface {
	return &qmssngrClient{
		WithLog:  logutil.WithLog{Log: log},
		http:     &http.Client{},
		url:      QMssngrURL,
		token:    token,
		replacer: replacer,
		users:    users,
		userFile: userFile,
	}
}

package messenger

import (
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
	TelegramURL = "https://api.telegram.org"
	TelegramID  = "telegram"
)

////////////////////////////////////////////////////////////////////////////////

type telegramClient struct {
	logutil.WithLog
	http *http.Client

	url string

	replacer *strings.Replacer

	users    string
	userFile string
}

func (t *telegramClient) SendMessage(
	ctx context.Context,
	chatID string,
	text string,
) error {
	if t.replacer != nil {
		text = t.replacer.Replace(text)
	}

	req, err := http.NewRequest("GET", t.url+"sendMessage", nil)
	if err != nil {
		return fmt.Errorf("SendMessage. Can't create request: %w", err)
	}

	q := req.URL.Query()
	q.Add("chat_id", chatID)
	q.Add("text", text)

	req.URL.RawQuery = q.Encode()

	resp, err := t.http.Do(req)
	if err != nil {
		return fmt.Errorf("SendMessage. Request error: %w", err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return fmt.Errorf("SendMessage. Read body error: %w", err)
	}

	m := &struct {
		Ok bool `json:"ok"`
	}{}

	if err := json.Unmarshal(body, m); err != nil {
		return fmt.Errorf("SendMessage. Unmarshal error: %w", err)
	}

	t.LogDbg(ctx, "[TG] ok: %v; body: %v", m.Ok, string(body))

	if !m.Ok {
		return fmt.Errorf("SendMessage. Bad response json: %v", string(body))
	}

	return nil
}

func (t *telegramClient) GetMessengerType() MessengerType {
	return Telegram
}

func (t *telegramClient) GetUserChatID(username string) string {
	return getUserChatID(username, t.users)
}

func (t *telegramClient) GetUsersFile() string {
	return t.userFile
}

////////////////////////////////////////////////////////////////////////////////

func NewTelegramClient(
	log nbs.Log,
	token string,
	replacer *strings.Replacer,
	users string,
	userFile string,
) MessengerClientIface {
	return &telegramClient{
		WithLog:  logutil.WithLog{Log: log},
		http:     &http.Client{},
		url:      fmt.Sprintf("%v/bot%v/", TelegramURL, token),
		replacer: replacer,
		users:    users,
		userFile: userFile,
	}
}

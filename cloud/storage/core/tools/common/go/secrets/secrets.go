package secrets

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"strings"

	"a.yandex-team.ru/library/go/yandex/yav/httpyav"
)

////////////////////////////////////////////////////////////////////////////////

type YcpConfig struct {
	Profiles map[string]string `json:"profiles"`
}

type Secrets struct {
	Z2ApiKey          string `json:"z2_api_key"`
	TelegramToken     string `json:"telegram_token"`
	OAuthToken        string `json:"oauth_token"`
	SolomonOAuthToken string `json:"solomon_oauth_token"`
	SolomonIamToken   string `json:"solomon_iam_token"`
	BotOAuthToken     string `json:"bot_token"`

	Ycp *YcpConfig `json:"ycp"`
}

////////////////////////////////////////////////////////////////////////////////

type SecretFilter struct {
	logger   io.Writer
	replacer *strings.Replacer
}

func (f SecretFilter) Write(p []byte) (n int, err error) {
	s := f.replacer.Replace(string(p))

	return f.logger.Write([]byte(s))
}

////////////////////////////////////////////////////////////////////////////////

func NewReplacer(
	s *Secrets,
	iamToken string,
) *strings.Replacer {
	repl := []string{
		s.Z2ApiKey, strings.Repeat("Z", len(s.Z2ApiKey)),
		s.TelegramToken, strings.Repeat("T", len(s.TelegramToken)),
		s.OAuthToken, strings.Repeat("O", len(s.OAuthToken)),
	}

	if len(iamToken) != 0 {
		repl = append(repl, iamToken, strings.Repeat("I", len(iamToken)))
	}

	return strings.NewReplacer(repl...)
}

func NewFilter(
	logger io.Writer,
	r *strings.Replacer,
) SecretFilter {

	return SecretFilter{
		logger:   logger,
		replacer: r,
	}
}

func GetDefaultSecretsPath() (string, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return "", err
	}
	return home + "/.nbs-tools/secrets.json", nil
}

func LoadFromFile(path string) (*Secrets, error) {
	secretsJSON, err := ioutil.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("can't read secrets: %w", err)
	}

	secrets := &Secrets{}
	if err := json.Unmarshal(secretsJSON, secrets); err != nil {
		return nil, fmt.Errorf("can't unmarshal secrets: %w", err)
	}

	return secrets, nil
}

////////////////////////////////////////////////////////////////////////////////

type YavSecrets map[string]string

func ReadSecretsFromYav(secretKey string, yavToken string) (YavSecrets, error) {
	client, err := httpyav.NewClient(httpyav.WithOAuthToken(yavToken))
	if err != nil {
		return nil, err
	}

	ctx := context.Background()
	ver, err := client.GetVersion(ctx, secretKey)
	if err != nil {
		return nil, err
	}

	output := make(map[string]string)
	for _, s := range ver.Version.Values {
		output[s.Key] = s.Value
	}

	return output, err
}

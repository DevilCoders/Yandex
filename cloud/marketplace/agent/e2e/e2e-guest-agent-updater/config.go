package e2eguestagentupdater

import (
	"context"
	"fmt"
	"time"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend/env"
)

type config struct {
	Token    string `config:"e2e_token,required"`
	CloudID  string `config:"e2e_cloud_id,required"`
	FolderID string `config:"e2e_folder_id,required"`
}

const agentSrc = "guest-agent-updater.exe"

var agentDest = fmt.Sprintf(`C:\temp\%v`, agentSrc)

// loadConfig return instance of config we build form file and environment variables.
func loadConfig() (*config, error) {
	c := config{}

	const configLoadTimeout = 10 * time.Second
	ctx, cancel := context.WithTimeout(context.Background(), configLoadTimeout)
	defer cancel()

	if err := confita.NewLoader(env.NewBackend()).Load(ctx, &c); err != nil {
		return nil, err
	}

	return &c, nil
}

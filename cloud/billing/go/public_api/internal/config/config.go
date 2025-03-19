package config

import (
	"context"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend/file"
)

type Config struct {
	ListenAddr            string `yaml:"listen_addr" config:"listen_addr,required"`
	StatusListenAddr      string `yaml:"status_listen_addr" config:"status_listen_addr,required"`
	ConsoleURL            string `yaml:"console_url" config:"console_url,required"`
	EnableJournald        bool   `yaml:"enable_journald" config:"enable_journald"`
	SSLCertificatePath    string `yaml:"ssl_certificate_path" config:"ssl_certificate_path,required"`
	SSLCertificateKeyPath string `yaml:"ssl_certificate_key_path" config:"ssl_certificate_key_path,required"`
	ProcessUserName       string `yaml:"process_user_name" config:"process_user_name,required"`
}

func LoadConfig(ctx context.Context, path string) (*Config, error) {
	config := Config{}

	loader := confita.NewLoader(file.NewBackend(path))
	err := loader.Load(ctx, &config)
	if err != nil {
		return nil, err
	}

	return &config, nil
}

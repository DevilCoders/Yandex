package main

type CliArgs struct {
	ConfigPath string
}

type AppConfig struct {
	NannyTokenFilename string           `yaml:"nanny_token_filename"`
	HTTPServer         ServerConfig     `yaml:"http_server"`
	CaptchaDuration    string           `yaml:"captcha_duration"`
	Endpoints          []EndpointConfig `yaml:"backends"`
	Services           []ServiceConfig  `yaml:"services"`
	Ruchkas            []RuchkaConfig   `yaml:"ruchkas"`
}

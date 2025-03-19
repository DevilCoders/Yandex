package keys

import "a.yandex-team.ru/cloud/mdb/mdb-disklock/internal/keys/aws"

type Config struct {
	Name       string     `json:"name" yaml:"name"`
	Type       string     `json:"type" yaml:"type"`
	Ciphertext string     `json:"ciphertext" yaml:"ciphertext"`
	AWSConfig  aws.Config `json:"aws_config" yaml:"aws_config"`
}

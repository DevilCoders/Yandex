package config

import "os"

func LoadEnvToString(name string, s *string) bool {
	val, ok := os.LookupEnv(name)
	if ok {
		*s = val
	}

	return ok
}

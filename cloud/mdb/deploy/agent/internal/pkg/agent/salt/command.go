package salt

import (
	"regexp"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/commander"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

var validName = regexp.MustCompile(`^[A-Za-z0-9]+`)

type Config struct {
	Binary string   `json:"binary" yaml:"binary"`
	Args   []string `json:"args" yaml:"args"`
}

func DefaultConfig() Config {
	return Config{
		Binary: "salt-call",
		Args: []string{
			"--out=highstate",
			"--state-out=changes",
			"--output-diff",
			"--log-file-level=info",
			"--log-level=quiet",
			"--retcode-passthrough",
		},
	}
}

func FormCommand(cfg Config, cmd commander.Command) (Job, error) {
	if !validName.MatchString(cmd.Name) {
		return Job{}, semerr.InvalidInputf("invalid command .Name should starts with alnum+: %q", cmd.Name)
	}
	for i, a := range cmd.Args {
		if !validName.MatchString(a) {
			return Job{}, semerr.InvalidInputf("invalid command .Args[%d] should starts with alnum+: %q", i, a)
		}
	}
	args := make([]string, len(cfg.Args), len(cfg.Args)+len(cmd.Args)+1)
	copy(args, cfg.Args)
	args = append(args, cmd.Name)
	args = append(args, cmd.Args...)

	if cmd.Options.Colored {
		args = append(args, "--force-color")
	}

	return Job{
		ID:   cmd.ID,
		Name: cfg.Binary,
		Args: args,
	}, nil
}

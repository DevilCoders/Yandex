package repos

type Config struct {
	Fileroots                    RepoConfig   `yaml:"fileroots"`
	FilerootsEnvsMountPathDevEnv string       `yaml:"fileroots_envs_mount_path_dev_env"`
	FilerootsEnvsMountPath       string       `yaml:"fileroots_envs_mount_path"`
	FilerootsSubrepos            []RepoConfig `yaml:"fileroots_subrepos"`
}

type RepoConfig struct {
	URI   string `yaml:"repo_uri"`
	Path  string `yaml:"repo_path"`
	Mount string `yaml:"mount_path"`
}

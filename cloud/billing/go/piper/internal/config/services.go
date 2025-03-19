package config

type Runner interface {
	Run() error
}

func (c *Container) GetResharderServices() ([]Runner, error) {
	if err := c.initResharder(); err != nil {
		return nil, err
	}

	runners := make([]Runner, 0, len(c.resharderServices))
	for _, s := range c.resharderServices {
		runners = append(runners, s)
	}
	return runners, nil
}

func (c *Container) GetDumperServices() ([]Runner, error) {
	if err := c.initDumper(); err != nil {
		return nil, err
	}

	runners := make([]Runner, 0, len(c.dumperServices))
	for _, s := range c.dumperServices {
		runners = append(runners, s)
	}
	return runners, nil
}

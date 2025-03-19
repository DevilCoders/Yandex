package main

import (
	"errors"
	"fmt"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"time"

	"github.com/cenkalti/backoff/v4"
)

const (
	defaultMaxRetries = 10
)

var (
	helpMessage = fmt.Sprintf(`
Usage: retry [options] <cmd>

Options:
	-i <value>  Set initial backoff interval (seconds) (default: %d)
	-r <value>  Set randomization factor (default: %.1f)
	-m <value>  Set multiplier (default: %.1f)
	-l <value>  Set max interval (seconds) (default: %d)
	-t <value>  Set max elapsed time (seconds) (default: %d)
	-n <value>  Set max retries (default: %d)
`,
		uint64(backoff.DefaultInitialInterval.Seconds()),
		backoff.DefaultRandomizationFactor,
		backoff.DefaultMultiplier,
		uint64(backoff.DefaultMaxInterval.Seconds()),
		uint64(backoff.DefaultMaxElapsedTime.Seconds()),
		defaultMaxRetries)
)

type config struct {
	initialInterval     time.Duration
	randomizationFactor float64
	multiplier          float64
	maxInterval         time.Duration
	maxElapsedTime      time.Duration
	maxRetries          uint64
	cmd                 []string
}

// We parse args this way to be able to use retry as "retry echo -n blablabla"
type parserState int

const (
	start parserState = iota
	parseInitialInterval
	parseRandomizationFactor
	parseMultiplier
	parseMaxInterval
	parseMaxElapsedTime
	parseMaxRetries
	parseCmd
)

func configure() (config, error) {
	conf := config{
		initialInterval:     backoff.DefaultInitialInterval,
		randomizationFactor: backoff.DefaultRandomizationFactor,
		multiplier:          backoff.DefaultMultiplier,
		maxInterval:         backoff.DefaultMaxInterval,
		maxElapsedTime:      backoff.DefaultMaxElapsedTime,
		maxRetries:          defaultMaxRetries,
	}

	state := start

	for _, token := range os.Args[1:] {
		if state == parseCmd {
			conf.cmd = append(conf.cmd, token)
		} else if state == start {
			if !strings.HasPrefix(token, "-") {
				state = parseCmd
				conf.cmd = append(conf.cmd, token)
			} else {
				switch token {
				case "-i":
					state = parseInitialInterval
				case "-r":
					state = parseRandomizationFactor
				case "-m":
					state = parseMultiplier
				case "-l":
					state = parseMaxInterval
				case "-t":
					state = parseMaxElapsedTime
				case "-n":
					state = parseMaxRetries
				default:
					return conf, errors.New("Unknown option: " + token)
				}
			}
		} else if state == parseInitialInterval {
			val, err := strconv.ParseInt(token, 10, 64)
			if err != nil {
				return conf, fmt.Errorf("error parsing initial interval: %w", err)
			}
			conf.initialInterval = time.Duration(val) * time.Second
			state = start
		} else if state == parseRandomizationFactor {
			val, err := strconv.ParseFloat(token, 64)
			if err != nil {
				return conf, fmt.Errorf("error parsing randomization factor: %w", err)
			}
			conf.randomizationFactor = val
			state = start
		} else if state == parseMultiplier {
			val, err := strconv.ParseFloat(token, 64)
			if err != nil {
				return conf, fmt.Errorf("error parsing multiplier: %w", err)
			}
			conf.multiplier = val
			state = start
		} else if state == parseMaxInterval {
			val, err := strconv.ParseInt(token, 10, 64)
			if err != nil {
				return conf, fmt.Errorf("error parsing max interval: %w", err)
			}
			conf.maxInterval = time.Duration(val) * time.Second
			state = start
		} else if state == parseMaxElapsedTime {
			val, err := strconv.ParseInt(token, 10, 64)
			if err != nil {
				return conf, fmt.Errorf("error parsing max elapsed time: %w", err)
			}
			conf.maxElapsedTime = time.Duration(val) * time.Second
			state = start
		} else if state == parseMaxRetries {
			val, err := strconv.ParseUint(token, 10, 64)
			if err != nil {
				return conf, fmt.Errorf("error parsing max retries: %w", err)
			}
			conf.maxRetries = val
			state = start
		}
	}

	if len(conf.cmd) == 0 {
		return conf, errors.New("empty command")
	}

	return conf, nil
}

func main() {
	conf, err := configure()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Configuration error: %v\n", err)
		fmt.Fprint(os.Stderr, helpMessage)
		os.Exit(1)
	}

	operation := func() error {
		fmt.Fprintf(os.Stderr, "Running command: %s\n", strings.Join(conf.cmd, " "))
		execPath, err := exec.LookPath(conf.cmd[0])
		if err != nil {
			return err
		}
		command := exec.Cmd{
			Path:   execPath,
			Args:   conf.cmd,
			Stderr: os.Stderr,
			Stdout: os.Stdout,
		}

		return command.Run()
	}

	b := backoff.NewExponentialBackOff()
	b.InitialInterval = conf.initialInterval
	b.RandomizationFactor = conf.randomizationFactor
	b.Multiplier = conf.multiplier
	b.MaxInterval = conf.maxInterval
	b.MaxElapsedTime = conf.maxElapsedTime
	b.Reset()

	err = backoff.Retry(operation, backoff.WithMaxRetries(b, conf.maxRetries))

	if err != nil {
		fmt.Fprintf(os.Stderr, "Command failed: %v\n", err)
		os.Exit(1)
	}
}

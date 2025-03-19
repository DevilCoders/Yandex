package cfgtypes

import (
	"strconv"
	"strings"
	"time"
)

type Seconds time.Duration

func (s Seconds) Duration() time.Duration {
	return time.Duration(s)
}

func (s Seconds) String() string {
	return time.Duration(s).String()
}

func (s Seconds) MarshalText() ([]byte, error) {
	d := time.Duration(s)
	return []byte(strconv.FormatFloat(d.Seconds(), 'f', -1, 64)), nil
}

func (s *Seconds) UnmarshalText(text []byte) error {
	value := string(text)

	if fl, err := strconv.ParseFloat(value, 64); err == nil {
		*s = Seconds(time.Second * time.Duration(fl))
		return nil
	}
	d, err := time.ParseDuration(value)
	if err != nil {
		return err
	}
	*s = Seconds(d)
	return nil
}

const (
	KiB = 1024
	MiB = 1024 * KiB
	GiB = 1024 * MiB
	TiB = 1024 * GiB
	PiB = 1024 * TiB
)

var unitMap = map[string]DataSize{"kib": KiB, "mib": MiB, "gib": GiB, "tib": TiB, "pib": PiB}

type DataSize int

func (s DataSize) Int() int {
	return int(s)
}

func (s DataSize) String() string {
	switch {
	case s%PiB == 0:
		return strconv.Itoa(int(s)/PiB) + "PiB"
	case s%TiB == 0:
		return strconv.Itoa(int(s)/TiB) + "TiB"
	case s%GiB == 0:
		return strconv.Itoa(int(s)/GiB) + "GiB"
	case s%MiB == 0:
		return strconv.Itoa(int(s)/MiB) + "MiB"
	case s%KiB == 0:
		return strconv.Itoa(int(s)/KiB) + "KiB"
	}
	return strconv.Itoa(int(s))
}

func (s DataSize) MarshalText() ([]byte, error) {
	return []byte(s.String()), nil
}

func (s *DataSize) UnmarshalText(text []byte) error {
	value := string(text)

	var multiplier DataSize = 1
	if len(value) > 3 {
		suf := value[len(value)-3:]
		if val, ok := unitMap[strings.ToLower(suf)]; ok {
			multiplier = val
			value = strings.TrimRight(value[:len(value)-3], " ")
		}
	}

	i, err := strconv.Atoi(value)
	if err != nil {
		return err
	}
	*s = multiplier * DataSize(i)
	return nil
}

// OverridableBool is special type representing bool in loadable configs. This type can distinguish cases of set false
// value by unmarshal method and default value. Used for correct values override by struct merges.
type OverridableBool uint

const (
	BoolDefault OverridableBool = iota
	BoolFalse
	BoolTrue
)

func (b OverridableBool) Bool() bool {
	return b == BoolTrue
}

func (b OverridableBool) MarshalText() ([]byte, error) {
	return []byte(strconv.FormatBool(b.Bool())), nil
}

func (b *OverridableBool) UnmarshalText(text []byte) error {
	value, err := strconv.ParseBool(string(text))
	if err != nil {
		return err
	}
	*b = BoolFalse
	if value {
		*b = BoolTrue
	}
	return nil
}

package origin

import (
	"time"
)

type YamlTime time.Time

func (t YamlTime) Unix() int {
	return int(time.Time(t).Unix())
}

const (
	dateLayout     = "2006-01-02"
	dateTimeLayout = "2006-01-02T15:04:05Z07:00"
)

func (t *YamlTime) UnmarshalYAML(unmarshal func(interface{}) error) error {
	var val string
	if err := unmarshal(&val); err != nil {
		return err
	}

	if timeVal, err := time.Parse(dateTimeLayout, val); err == nil {
		*t = YamlTime(timeVal)
		return nil
	}

	timeVal, err := time.Parse(dateLayout, val)
	*t = YamlTime(timeVal)
	return err
}

func (t YamlTime) MarshalYAML() (interface{}, error) {
	tt := time.Time(t).In(time.Local)
	if tt.Equal(tt.Truncate(time.Hour * 24)) {
		return tt.Format(dateLayout), nil
	}
	return tt.Format(dateTimeLayout), nil
}

package solomon

import (
	"reflect"
	"strings"

	"a.yandex-team.ru/admins/combaine/senders"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// TaskConfig object
type TaskConfig struct {
	L                   log.Logger
	prefix              string
	Timestamp           int64
	useSpackCompression bool
	useSpack            bool
	token               string
	API                 string   `msgpack:"api"`
	Timeout             int      `msgpack:"timeout"`
	Project             string   `msgpack:"project"`
	Cluster             string   `msgpack:"cluster"`
	Service             string   `msgpack:"service"`
	Fields              []string `msgpack:"Fields"`
	Schema              []string `msgpack:"schema"`
}

func (t *TaskConfig) updateByServerConfig(c *Config) {
	if len(t.Fields) == 0 {
		t.Fields = defaultFields
	}
	if t.Timeout == 0 {
		t.Timeout = c.Timeout
	}
	if t.API == "" {
		t.API = c.API
	}
	t.useSpackCompression = c.UseSpackCompression
	t.useSpack = c.UseSpack
	t.token = c.Token

	if strings.ContainsRune(t.Service, '.') {
		aPrefix := strings.SplitN(t.Service, ".", 2)
		t.Service = aPrefix[0]
		t.prefix = aPrefix[1]
	}

}

// Validate the task config
func (t *TaskConfig) Validate() error {
	var notSet []string

	if t.token == "" {
		return xerrors.New("Authorization token not provided")
	}
	if t.Project == "" {
		notSet = append(notSet, "project")
	}
	if t.Cluster == "" {
		notSet = append(notSet, "cluster")
	}
	if t.Service == "" {
		notSet = append(notSet, "service")
	}
	if len(notSet) > 0 {
		return xerrors.Errorf("required field is not set: %v", notSet)
	}
	return nil
}

func (t *TaskConfig) dumpSensor(reg *solomon.Registry, path senders.NameStack, value interface{}) error {
	var sensorValue float64

	switch v := value.(type) {
	case float32:
		sensorValue = float64(v)
	case float64:
		sensorValue = v
	case int:
		sensorValue = float64(v)
	case int8:
		sensorValue = float64(v)
	case int16:
		sensorValue = float64(v)
	case int32:
		sensorValue = float64(v)
	case int64:
		sensorValue = float64(v)
	case uint:
		sensorValue = float64(v)
	case uint8:
		sensorValue = float64(v)
	case uint16:
		sensorValue = float64(v)
	case uint32:
		sensorValue = float64(v)
	case uint64:
		sensorValue = float64(v)
	default:
		return xerrors.Errorf("Sensor is Not a Number: %s=%v", strings.Join(path, "."), value)
	}
	name, labels, err := t.collectLabels(path)
	if err != nil {
		return err
	}
	var subregistry metrics.Registry = reg
	if len(labels) > 0 {
		subregistry = reg.WithTags(labels)
	}
	subregistry.Gauge(name).Set(sensorValue)
	return nil
}

func (t *TaskConfig) collectLabels(path senders.NameStack) (string, map[string]string, error) {
	labels := make(map[string]string)
	var name string

	if len(t.Schema) > 0 {
		metricName := path[0]
		values := strings.SplitN(metricName, ".", len(t.Schema)+1)
		if len(t.Schema)+1 != len(values) {
			return "", nil, xerrors.Errorf("Unable to send %+v(size %d) with schema %+v(size %d)",
				values, len(values), t.Schema, len(t.Schema))
		}

		for i := 0; i < len(t.Schema); i++ {
			labels[t.Schema[i]] = values[i]
		}
		if t.prefix != "" {
			labels["prefix"] = t.prefix
		}

		name = strings.Join(values[len(t.Schema):], ".")
		suffix := strings.Join(path[1:], ".")

		if suffix != "" {
			name = name + "." + suffix
		}
	} else {
		name = strings.Join(path, ".")
		if t.prefix != "" {
			name = t.prefix + "." + name
		}
	}
	return name, labels, nil
}

func (t *TaskConfig) dumpSlice(reg *solomon.Registry, path senders.NameStack, rv reflect.Value) error {

	if len(t.Fields) == 0 || len(t.Fields) != rv.Len() {
		msg := xerrors.Errorf("Unable to send a slice. Fields len %d, len of value %d", len(t.Fields), rv.Len())
		t.L.Errorf("%s", msg)
		return msg
	}

	for i := 0; i < rv.Len(); i++ {
		path.Push(t.Fields[i])
		err := t.dumpSensor(reg, path, rv.Index(i).Interface())
		path.Pop()
		if err != nil {
			return err
		}
	}
	return nil
}

func (t *TaskConfig) dumpMap(reg *solomon.Registry, path senders.NameStack, rv reflect.Value) error {
	var (
		err error
	)

	for _, key := range rv.MapKeys() {
		path.Push(key.String())
		itemInterface := rv.MapIndex(key).Interface()
		reflectValue := reflect.ValueOf(itemInterface)
		valueKind := reflectValue.Kind()
		t.L.Debugf("Item of key %s is: %v", key, valueKind)

		switch valueKind {
		case reflect.Slice, reflect.Array:
			err = t.dumpSlice(reg, path, reflectValue)
		case reflect.Map:
			err = t.dumpMap(reg, path, reflectValue)
		default:
			err = t.dumpSensor(reg, path, itemInterface)
		}

		if err != nil {
			return err
		}
		path.Pop()
	}
	return err
}

func (t *TaskConfig) dumpSensors(host string, item *senders.Payload) (*solomon.Registry, error) {
	regOpts := solomon.NewRegistryOpts().SetTags(map[string]string{"host": host})
	reg := solomon.NewRegistry(regOpts)

	aggname := item.Tags["aggregate"]

	rv := reflect.ValueOf(item.Result)
	t.L.Debugf("Handle payload type: %s.%s -> %s", host, aggname, rv.Kind())

	if rv.Kind() != reflect.Map {
		err := xerrors.Errorf("Value of group should be dict, skip: %v", rv)
		t.L.Errorf("%s", err)
		return nil, err
	}
	err := t.dumpMap(reg, []string{}, rv)
	if err != nil {
		err = xerrors.Errorf("%s bad value for %s: %#v", err, aggname, item.Result)
		t.L.Errorf("%s", err)
		return nil, err
	}
	return reg, nil
}

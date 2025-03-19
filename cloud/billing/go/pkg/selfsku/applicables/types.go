package applicables

// ResolvingRuleOption is helper type for serialization delights
type ResolvingRuleOption struct {
	Null        bool
	EmptyString bool
	Str         string
	Number      float64
	IsTrue      bool
	IsFalse     bool
	ExistsRule  bool
	ExistsValue bool
}

func (o ResolvingRuleOption) MarshalYAML() (interface{}, error) {
	switch {
	case o.Null:
		return nil, nil
	case o.EmptyString:
		return "", nil
	case o.Str != "":
		return o.Str, nil
	case o.IsFalse:
		return false, nil
	case o.IsTrue:
		return true, nil
	case o.ExistsRule:
		var val struct {
			Exists bool `yaml:"exists"`
		}
		val.Exists = o.ExistsValue
		return val, nil
	default:
		return o.Number, nil
	}
}

func (o *ResolvingRuleOption) UnmarshalYAML(unmarshal func(interface{}) error) error {
	{
		var val interface{}
		if err := unmarshal(&val); err != nil {
			return err
		}
		if val == nil {
			o.Null = true
			return nil
		}
	}
	{
		var val bool
		if err := unmarshal(&val); err == nil {
			if val {
				o.IsTrue = true
				return nil
			}
			o.IsFalse = true
			return nil
		}
	}
	{
		var val float64
		if err := unmarshal(&val); err == nil {
			o.Number = val
			return nil
		}
	}
	{
		var val string
		if err := unmarshal(&val); err == nil {
			o.Str = val
			o.EmptyString = val == ""
			return nil
		}
	}

	var val struct {
		Exists bool `yaml:"exists"`
	}
	if err := unmarshal(&val); err != nil {
		return err
	}
	o.ExistsRule = true
	o.ExistsValue = val.Exists
	return nil
}

// QuantumConstant is for the dummy field wit only value `1`
type QuantumConstant struct{}

func (QuantumConstant) MarshalYAML() (interface{}, error) {
	return 1, nil
}

func (QuantumConstant) UnmarshalYAML(unmarshal func(interface{}) error) error {
	return nil
}

// DummyString is placeholder for legacy values
type DummyString struct{}

func (DummyString) MarshalYAML() (interface{}, error) {
	return "", nil
}

func (DummyString) UnmarshalYAML(unmarshal func(interface{}) error) error {
	return nil
}

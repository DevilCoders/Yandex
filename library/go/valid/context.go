package valid

type ValidationCtx struct {
	validators map[string]ValidatorFunc
}

// NewValidationCtx returns new validation context.
// Validation context holds a set of ValidatorFunc and can be used
// to limit scope of object validation rules.
func NewValidationCtx() *ValidationCtx {
	return &ValidationCtx{
		validators: make(map[string]ValidatorFunc),
	}
}

// Add writes down new validator into validation context.
// New validator can override already existing in context by name.
func (c *ValidationCtx) Add(name string, validator ValidatorFunc) {
	c.validators[name] = validator
}

// Set replaces existing validators with new ones.
func (c *ValidationCtx) Set(m map[string]ValidatorFunc) {
	c.validators = m
}

// Get returns validator from validation context by name.
func (c *ValidationCtx) Get(name string) (f ValidatorFunc, ok bool) {
	f, ok = c.validators[name]
	return
}

// Merge validators from ctx into current validation context
// New validators can override already existing ones in context by name.
func (c *ValidationCtx) Merge(ctx *ValidationCtx) {
	for k, v := range ctx.validators {
		c.validators[k] = v
	}
}

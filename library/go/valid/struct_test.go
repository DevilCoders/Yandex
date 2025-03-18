package valid_test

import (
	"reflect"
	"testing"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

var _ valid.Validator = new(TestSelfValidator)
var _ valid.Validator = new(TestSelfValidatorWithProceed)

type TestNullAccount struct {
	ID       int64
	Username string
	Email    string
}

func (na TestNullAccount) IsZero() bool {
	return na.ID == 0
}

type TestSelfValidator struct {
	TvmTicket []byte
}

func (sv TestSelfValidator) Validate(_ *valid.ValidationCtx) (bool, error) {
	if len(sv.TvmTicket) < 10 &&
		sv.TvmTicket[0] != '1' {
		return false, valid.ErrInvalidChecksum
	}
	return true, nil
}

type TestSelfValidatorWithProceed struct {
	TvmTicket []byte
	UserUID   string `valid:"uuid4"`
}

func (sv TestSelfValidatorWithProceed) Validate(_ *valid.ValidationCtx) (bool, error) {
	if len(sv.TvmTicket) < 10 &&
		sv.TvmTicket[0] != '1' {
		return true, valid.ErrInvalidChecksum
	}
	return true, nil
}

func validateTestNullAccount(value reflect.Value, _ string) (err error) {
	na, ok := value.Interface().(TestNullAccount)
	if !ok {
		return valid.ErrBadParams
	}

	var errs valid.Errors
	if err := valid.StringLen(na.Username, 5, 255); err != nil {
		errs = append(errs, xerrors.Errorf("Username: %w", err))
	}
	if na.Email == "" {
		errs = append(errs, xerrors.Errorf("Email: %w", valid.ErrEmptyString))
	}

	if len(errs) != 0 {
		err = valid.ErrValidation.Wrap(errs)
	}

	return
}

func TestStruct_nilValidationContextPanic(t *testing.T) {
	assert.Panics(t, func() {
		_ = valid.Struct(nil, struct {
			Name string `valid:"min=10"`
		}{
			Name: "test",
		})
	})
}

func TestStruct(t *testing.T) {
	testCases := []struct {
		name       string
		ctx        *valid.ValidationCtx
		param      interface{}
		expectErrs valid.Errors
	}{
		// VALID
		{
			name: "non-struct",
			ctx: func() *valid.ValidationCtx {
				return valid.NewValidationCtx()
			}(),
			param:      "shimba-boomba",
			expectErrs: valid.Errors{valid.ErrStructExpected},
		},
		{
			name: "empty_struct",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				return ctx
			}(),
			param: struct {
			}{},
			expectErrs: nil,
		},
		{
			name: "struct_with_private_fields",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				return ctx
			}(),
			param: struct {
				id string `valid:"uuid4"`
			}{
				id: uuid.Must(uuid.NewV4()).String(),
			},
			expectErrs: nil,
		},
		{
			name: "struct_with_skipped_fields",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				return ctx
			}(),
			param: struct {
				ID string `valid:"-"`
			}{
				ID: uuid.Must(uuid.NewV4()).String(),
			},
			expectErrs: nil,
		},
		{
			name: "struct_with_nil_interface",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				return ctx
			}(),
			param: struct {
				Item interface{}
			}{},
			expectErrs: nil,
		},
		{
			name: "valid_struct_with_basic_validator",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				return ctx
			}(),
			param: struct {
				ID string `valid:"uuid4"`
			}{
				ID: uuid.Must(uuid.NewV4()).String(),
			},
			expectErrs: nil,
		},
		{
			name: "valid_struct_with_custom_validator",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				ctx.Add("null_account", validateTestNullAccount)
				return ctx
			}(),
			param: struct {
				ID      string          `valid:"uuid4"`
				Account TestNullAccount `valid:"null_account"`
			}{
				ID: uuid.Must(uuid.NewV4()).String(),
				Account: TestNullAccount{
					ID:       12345,
					Username: "my_long_valid_username",
					Email:    "valid_email@yandex.ru",
				},
			},
			expectErrs: nil,
		},
		{
			name: "valid_struct_with_empty_fields",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				ctx.Add("credit_card", valid.WrapValidator(valid.CreditCard))
				ctx.Add("null_account", validateTestNullAccount)
				return ctx
			}(),
			param: struct {
				ID            string          `valid:"uuid4"`
				PaymentMethod string          `valid:"credit_card,omitempty"`
				Account       TestNullAccount `valid:"null_account,omitempty"`
				BackupCode    *string         `valid:"uuid4,omitempty"`
			}{
				ID:            uuid.Must(uuid.NewV4()).String(),
				PaymentMethod: "",
				Account:       TestNullAccount{},
			},
			expectErrs: nil,
		},
		{
			name: "valid_struct_with_validator_interface",
			ctx:  valid.NewValidationCtx(),
			param: TestSelfValidator{
				TvmTicket: []byte("1234567890"),
			},
			expectErrs: nil,
		},
		{
			name: "valid_struct_with_param_fields",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				ctx.Add("credit_card", valid.WrapValidator(valid.CreditCard))
				ctx.Add("null_account", validateTestNullAccount)
				ctx.Add("min", valid.Min)
				ctx.Add("max", valid.Max)
				return ctx
			}(),
			param: struct {
				ID            string          `valid:"uuid4"`
				PaymentMethod string          `valid:"credit_card,omitempty"`
				Account       TestNullAccount `valid:"null_account,omitempty"`
				Balance       int             `valid:"min=10,max=12"`
			}{
				ID:            uuid.Must(uuid.NewV4()).String(),
				PaymentMethod: "",
				Account:       TestNullAccount{},
				Balance:       11,
			},
			expectErrs: nil,
		},
		{
			name: "valid_struct_with_nested_struct",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				ctx.Add("credit_card", valid.WrapValidator(valid.CreditCard))
				ctx.Add("null_account", validateTestNullAccount)
				ctx.Add("min", valid.Min)
				ctx.Add("max", valid.Max)
				ctx.Add("isbn", valid.WrapValidator(valid.ISBN))
				return ctx
			}(),
			param: struct {
				ID            string          `valid:"uuid4"`
				PaymentMethod string          `valid:"credit_card,omitempty"`
				Account       TestNullAccount `valid:"null_account,omitempty"`
				Balance       int             `valid:"min=10,max=12"`
				Metadata      struct {
					LastBook string `valid:"isbn,omitempty"`
				}
			}{
				ID:            uuid.Must(uuid.NewV4()).String(),
				PaymentMethod: "",
				Account:       TestNullAccount{},
				Balance:       11,
				Metadata: struct {
					LastBook string `valid:"isbn,omitempty"`
				}{
					LastBook: "9781234567897",
				},
			},
			expectErrs: nil,
		},
		{
			name: "valid_struct_with_iterables",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				ctx.Add("null_account", validateTestNullAccount)
				ctx.Add("min_len", valid.Min)
				return ctx
			}(),
			param: struct {
				DeviceUUID string              `valid:"uuid4,omitempty"`
				Accounts   []TestSelfValidator `valid:"omitempty"`
				Tags       []string            `valid:"min_len=2,omitempty"`
				Comments   map[string]string   `valid:"min_len=5,omitempty"`
			}{
				DeviceUUID: uuid.Must(uuid.NewV4()).String(),
				Accounts: []TestSelfValidator{
					{TvmTicket: []byte("1234567890")},
					{TvmTicket: []byte("1098765432")},
				},
				Tags: []string{"ololo", "trololo"},
			},
			expectErrs: nil,
		},
		// INVALID
		{
			name: "invalid_struct_with_basic_validator",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				return ctx
			}(),
			param: struct {
				ID string `valid:"uuid4"`
			}{
				ID: "some_non_uuid_string",
			},
			expectErrs: valid.Errors{
				valid.ErrInvalidStringLength,
			},
		},
		{
			name: "invalid_struct_with_paramed_validator",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("min", valid.Min)
				return ctx
			}(),
			param: struct {
				ID      string
				Balance int `valid:"min=10"`
			}{
				ID:      uuid.Must(uuid.NewV4()).String(),
				Balance: 5,
			},
			expectErrs: valid.Errors{
				valid.ErrLesserValue,
			},
		},
		{
			name: "invalid_struct_with_paramed_validators_pair",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("min", valid.Min)
				ctx.Add("max", valid.Max)
				return ctx
			}(),
			param: struct {
				ID      string
				Balance int `valid:"min=10,max=15"`
				Debt    int `valid:"min=0,max=999"`
			}{
				ID:      uuid.Must(uuid.NewV4()).String(),
				Balance: 0,
				Debt:    1001,
			},
			expectErrs: valid.Errors{
				valid.ErrLesserValue,
				valid.ErrGreaterValue,
			},
		},
		{
			name: "invalid_struct_with_custom_validator",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				ctx.Add("null_account", validateTestNullAccount)
				return ctx
			}(),
			param: struct {
				ID      string          `valid:"uuid4"`
				Account TestNullAccount `valid:"null_account"`
			}{
				ID: "some_non_uuid_string",
				Account: TestNullAccount{
					ID:       12345,
					Username: "nope",
					Email:    "",
				},
			},
			expectErrs: valid.Errors{
				valid.ErrInvalidStringLength,
				valid.ErrValidation.Wrap(
					valid.Errors{
						xerrors.Errorf("Username: %w", valid.ErrStringTooShort),
						xerrors.Errorf("Email: %w", valid.ErrEmptyString),
					},
				),
			},
		},
		{
			name: "invalid_struct_with_validator_interface",
			ctx:  valid.NewValidationCtx(),
			param: TestSelfValidator{
				TvmTicket: []byte("00000"),
			},
			expectErrs: valid.Errors{valid.ErrInvalidChecksum},
		},
		{
			name: "valid_struct_with_invalid_nested_struct",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				ctx.Add("credit_card", valid.WrapValidator(valid.CreditCard))
				ctx.Add("null_account", validateTestNullAccount)
				ctx.Add("min", valid.Min)
				ctx.Add("max", valid.Max)
				ctx.Add("isbn", valid.WrapValidator(valid.ISBN))
				return ctx
			}(),
			param: struct {
				ID            string          `valid:"uuid4"`
				PaymentMethod string          `valid:"credit_card,omitempty"`
				Account       TestNullAccount `valid:"null_account,omitempty"`
				Balance       int             `valid:"min=10,max=12"`
				Metadata      struct {
					LastBook string `valid:"isbn,omitempty"`
				}
			}{
				ID:            uuid.Must(uuid.NewV4()).String(),
				PaymentMethod: "",
				Account:       TestNullAccount{},
				Balance:       11,
				Metadata: struct {
					LastBook string `valid:"isbn,omitempty"`
				}{
					LastBook: "7987654312345",
				},
			},
			expectErrs: valid.Errors{
				valid.ErrInvalidISBN,
			},
		},
		{
			name: "invalid_struct_with_iterables",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				ctx.Add("null_account", validateTestNullAccount)
				ctx.Add("min_len", valid.Min)
				ctx.Add("max_len", valid.Max)
				return ctx
			}(),
			param: struct {
				DeviceUUID string              `valid:"uuid4,omitempty"`
				Accounts   []TestSelfValidator `valid:"omitempty"`
				Tags       []string            `valid:"max_len=2,omitempty"`
				Comments   map[string]string   `valid:"min_len=2"`
			}{
				DeviceUUID: uuid.Must(uuid.NewV4()).String(),
				Accounts: []TestSelfValidator{
					{TvmTicket: []byte("trololo")},
					{TvmTicket: []byte("ololo")},
				},
				Tags: []string{"shimba", "boomba", "looken"},
			},
			expectErrs: valid.Errors{
				valid.ErrInvalidChecksum,
				valid.ErrInvalidChecksum,
				valid.ErrGreaterValue,
				valid.ErrLesserValue,
			},
		},
		{
			name: "invalid_struct_with_proceed",
			ctx: func() *valid.ValidationCtx {
				ctx := valid.NewValidationCtx()
				ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
				ctx.Add("null_account", validateTestNullAccount)
				ctx.Add("min_len", valid.Min)
				ctx.Add("max_len", valid.Max)
				return ctx
			}(),
			param: TestSelfValidatorWithProceed{
				TvmTicket: []byte("trololo"),
				UserUID:   "ololo",
			},
			expectErrs: valid.Errors{
				valid.ErrInvalidChecksum,
				valid.ErrInvalidStringLength,
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			errs := valid.Struct(tc.ctx, tc.param)
			if tc.expectErrs == nil {
				assert.NoError(t, errs)
			} else {
				assert.IsType(t, valid.Errors{}, errs)
				assert.EqualError(t, errs, tc.expectErrs.Error())
			}
		})
	}
}

func TestNestedValidationFieldErrorReturned(t *testing.T) {
	ctx := valid.NewValidationCtx()
	ctx.Add("required", valid.WrapValidator(func(s string) error {
		if len(s) == 0 {
			return xerrors.New("value is required")
		}
		return nil
	}))

	type TestSelfValidatorWithTags struct {
		ID string `valid:"required"`
	}

	type TestStructWithChild struct {
		GrandChild TestSelfValidatorWithTags
	}

	type TestStructWithGrandChild struct {
		Child TestStructWithChild
	}

	param := TestStructWithGrandChild{
		Child: TestStructWithChild{
			GrandChild: TestSelfValidatorWithTags{
				ID: "",
			},
		},
	}

	errs := valid.Struct(ctx, param).(valid.Errors)
	if len(errs) != 1 {
		t.Error("Struct method has invalid contract")
	}

	var ferr valid.FieldError
	if xerrors.As(errs[0], &ferr) {
		fullPath := ferr.Path() + "." + ferr.Field()
		assert.Equal(t, fullPath, "Child.GrandChild.ID")
		assert.Equal(t, ferr.Error(), "value is required")
	} else {
		t.Error("Struct method has invalid contract")
	}
}

type SimpleTestStructWithValidate struct {
	Name string
}

func (sv SimpleTestStructWithValidate) Validate(_ *valid.ValidationCtx) (bool, error) {
	return true, nil
}

func TestStructWithNilPointerValidation(t *testing.T) {
	type TestStructWithPointer struct {
		Child *SimpleTestStructWithValidate `valid:"omitempty"`
	}

	s := TestStructWithPointer{}

	ctx := valid.NewValidationCtx()
	err := valid.Struct(ctx, s)
	assert.NoError(t, err)
}

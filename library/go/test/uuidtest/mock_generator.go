package uuidtest

import "github.com/gofrs/uuid"

var _ uuid.Generator = MockGenerator{}

// MockGenerator allows you to control UUID generation mechanisms in tests.
// Example:
//     func TestMyFunc(t *testing.T) {
//         origGen := uuid.DefaultGenerator
//         defer func() {
//             uuid.DefaultGenerator = origGen
//         }()
//
//         expected := uuid.FromString("62c15d7f-b99e-475c-8a5b-61c42f28dc6e")
//         uuid.DefaultGenerator = uuidtest.MockGenerator{
//             NewV4Func: func() (uuid.UUID, error) {
//                 return expected
//             }
//         }
//
//         res := myFuncWithUUID()
//         assert.Equals(t, expected, res)
//     }
type MockGenerator struct {
	NewV1Func func() (uuid.UUID, error)
	NewV3Func func(ns uuid.UUID, name string) uuid.UUID
	NewV4Func func() (uuid.UUID, error)
	NewV5Func func(ns uuid.UUID, name string) uuid.UUID
	NewV6Func func() (uuid.UUID, error)
	NewV7Func func(p uuid.Precision) (uuid.UUID, error)
}

func (mg MockGenerator) NewV1() (uuid.UUID, error) {
	if mg.NewV1Func == nil {
		return uuid.NewV1()
	}
	return mg.NewV1Func()
}

func (mg MockGenerator) NewV3(ns uuid.UUID, name string) uuid.UUID {
	if mg.NewV3Func == nil {
		return uuid.NewV3(ns, name)
	}
	return mg.NewV3Func(ns, name)
}

func (mg MockGenerator) NewV4() (uuid.UUID, error) {
	if mg.NewV4Func == nil {
		return uuid.NewV4()
	}
	return mg.NewV4Func()
}

func (mg MockGenerator) NewV5(ns uuid.UUID, name string) uuid.UUID {
	if mg.NewV5Func == nil {
		return uuid.NewV5(ns, name)
	}
	return mg.NewV5Func(ns, name)
}

func (mg MockGenerator) NewV6() (uuid.UUID, error) {
	if mg.NewV6Func == nil {
		return uuid.NewV6()
	}
	return mg.NewV6Func()
}

func (mg MockGenerator) NewV7(p uuid.Precision) (uuid.UUID, error) {
	if mg.NewV7Func == nil {
		return uuid.NewV7(p)
	}
	return mg.NewV7Func(p)
}

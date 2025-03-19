package tvm

type AuthResult struct {
	isAuthenticated bool
	serviceID       uint32
}

func (r AuthResult) IsAuthenticated() bool {
	return r.isAuthenticated
}

func (r AuthResult) ServiceID() uint32 {
	return r.serviceID
}

func NewResult(serviceID uint32) AuthResult {
	return AuthResult{
		isAuthenticated: true,
		serviceID:       serviceID,
	}
}

func NewAnonymousResult() AuthResult {
	return AuthResult{
		isAuthenticated: false,
	}
}

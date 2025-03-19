package api

import (
	"encoding/json"
	"net/http"
)

// UsersAll - TODO
func (srv *Server) UsersAll(w http.ResponseWriter, r *http.Request) {
	//shs, _ := mongoclient.GetShifts(1569919834, "5d90c2192e96c977b76822ee", true)
	shs, _ := srv.Config.Mongo.GetShiftsByIDs([]string{"5dc17246fb4717da9899ac32", "5dc17246fb4717da9899ac34"})
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(shs)
}

// User - TODO
func (srv *Server) User(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	resp := make(map[string]interface{})
	if s, ok := ctx.Value(CtxStatus).(val); ok {
		switch s {
		case CtxAuthFailed:
			resp["status"] = "Failed"
			w.WriteHeader(http.StatusForbidden)
		case CtxAuthHeaderIncorrect:
			resp["status"] = "Incorect"
			w.WriteHeader(http.StatusBadRequest)
		case CtxAuthNoHeader:
			resp["status"] = "No header"
			w.WriteHeader(http.StatusBadRequest)
		case CtxAuthSuccess:
			resp["CtxUID"] = ctx.Value(CtxUID).(uint64)
			resp["CtxLogin"] = ctx.Value(CtxLogin).(string)
			resp["Scope"] = ctx.Value(CtxScopes).([]string)
		}
	}
	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(resp)
}

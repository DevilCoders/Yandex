package service

//Wall-E project settings
type Walle struct {
	Active *bool `json:"active,omitempty"` // VERIFY: is ptr
	Extra  struct {
		Type string `json:"type,omitempty"` //То, ради чего мы и собрались
	} `json:"extra,omitempty"`
	ID      string `json:"id,omitempty"`      //slag проекта
	Project string `json:"project,omitempty"` //Полное название проекта
	Queue   string `json:"queue,omitempty"`   //Очередь для burne-тикетов
	Summary string `json:"summary,omitempty"` //Содержание тикета, помимо стандартного
}

func (w *Walle) IsActive() bool {
	if w.Active == nil {
		return false
	}
	return *w.Active
}

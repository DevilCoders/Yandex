package service

type StTemplate struct {
	Active *bool  `json:"active,omitempty"` // VERIFY: is ptr
	Queue  string `json:"queue,omitempty"`  // VERIFY: get by slug //Имя очереди
	RuleID int    `json:"ruleid,omitempty"` // VERIFY: get by id //ID Темплейта
}

type StCreation struct {
	Managed *bool             `json:"managed,omitempty"` // VERIFY: is ptr // Создаём ли тикеты, если false, всё равно апдейтим
	Mode    string            `json:"mode,omitempty"`    // VERIFY: enum // shift - привязно к шифтам, schedule - по расписанию: в определенное время с повторами через заданное кол-во часов, time - в определённое время, period - через промежуток времени
	KwArgs  map[string]string `json:"kwargs,omitempty"`  // deprecated // shift - order; time - weekday, duration; period - days, weeks
}

type StDuty struct {
	Active     *bool       `json:"active,omitempty"`     // VERIFY: is ptr
	Continious *bool       `json:"continious,omitempty"` // VERIFY: is ptr //Флаг, стоит ли пугаться, увидев неправильный тикет
	Component  string      `json:"component,omitempty"`  // VERIFY: get by slug //ST-specific shit
	Last       string      `json:"last,omitempty"`       // VERIFY: no //Предыдущий тикет
	Queue      string      `json:"queue,omitempty"`      // VERIFY: get by slug //Имя очереди
	Creation   *StCreation `json:"creation,omitempty"`
}

type Startrack struct { //Тикеты и настройки очереди
	Active   *bool       `json:"active,omitempty"` // VERIFY: is ptr
	Template *StTemplate `json:"template,omitempty"`
	Duty     *StDuty     `json:"duty,omitempty"`
}

func (st *Startrack) IsActive() bool {
	if st.Active == nil {
		return false
	}
	return *st.Active
}

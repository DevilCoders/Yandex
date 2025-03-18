var CBB = CBB || {}

CBB.StaffPeople = function() {

  function getStaffSuggest(request, response) {
    let url = `https://search.yandex-team.ru/suggest/?version=2&layers=people&people.per_page=10&people.query=i_is_dismissed:0&text=${request.term}`

    $.ajax({
      type: 'GET',
      url: url,
      dataType: "json",
      xhrFields: {
        withCredentials: true
      }
    }).done(function(data) {
      let result = []

      for (const item of data.people.result) {
        result.push({label: item.title, login: item.login, department: item.department_name})
      }
      response(result)
    }).fail(function(data) {
      response([])
    })
  }

  function asyncLoadStaffData(loginList) {
    const list = Array.isArray(loginList) ? loginList : [loginList]

    if (!list.length) {
      return new Promise((resolve, reject) => resolve([]))
    }

    const logins = list.join(',')
    const url = `/staff/${logins}`

    return $.ajax({
        type: 'GET',
        url: url,
        dataType: "json",
        xhrFields: {
          withCredentials: true
    }})
  }

  class PeopleView {
    /*
      parentElemId: a div, where to put a list of persons

      addBtnId: a button to add a new person to the list

      initialData: an object like the following:
        {
          unchanged: [ <login>, ...],
          added: [ <login>, ... ],
          deleted: [ <login>, ... ]
        }

    */
    static _newLoginData(props) {
      let result = {
          login: null,
          justAdded: false,
          deleted: false,
          staffData: null
      }

      return Object.assign({}, result, props)
    }

    constructor(parentElemId, addBtnId, initialData, editMode) {
      this.elemId = parentElemId
      this.divList = $('<div></div>')
      this.addBtnId = addBtnId
      this.data = {} // a collection of LoginData()
      this.editMode = editMode

      this._fillInitialData(initialData)

      this.ready = new Promise((resolve, reject) => {
        asyncLoadStaffData(Object.keys(this.data))
          .then(
            result => {
              for(const personData of result.result) {
                this.data[personData.login].staffData = personData
              }
              resolve()
            },
            error => {
              reject(Error("Не удалось загрузить данные со стаффа."))
            }
       )})
    }

    _fillInitialData(initialData) {
      if (!initialData) {
        return
      }

      initialData.unchanged.forEach(item => { this.data[item] = PeopleView._newLoginData({login: item})})
      initialData.added.forEach(item => { this.data[item] = PeopleView._newLoginData({login: item, justAdded: true})})
      initialData.deleted.forEach(item => { this.data[item] = PeopleView._newLoginData({login: item, deleted: true})})
    }

    _asyncAddLogin(login) {
      this.data[login] = {'login': login, justAdded: true, deleted: false, staffData: null}

      return new Promise((resolve, reject) => {
        asyncLoadStaffData(login)
        .then(
          result => {
            this.data[login].staffData = result.result[0]
            resolve()
          },
          error => {
            reject(Error("Не удалось загрузить данные со стаффа."))
          }
        )})
    }

    _removeLogin(login) {
      if (this.data[login].justAdded) {
        delete this.data[login]
      } else {
        this.data[login].deleted = true
      }
    }

    _createListRow(login, staffData) {
      let href = `https://staff.yandex-team.ru/${login}`
      let removeButton = this.editMode ?
                          `<button type="button" class="btn btn-link btn-xs"><span class="glyphicon glyphicon-remove" aria-hidden="true"></span></button>`
                          : '<div></div>'
      let html = `<div>
                    <a href="${href}" data-login="${login}">
                      ${staffData.name.first.ru} ${staffData.name.last.ru}
                    </a>
                    ${removeButton}
                  </div>`

      let elem = $(html)
      let loginData = this.data[login]

      if (loginData.deleted) {
        elem.addClass('people-item-deleted')
        $('button', elem).remove()
        return elem
      }

      if (this.data[login].justAdded) {
        elem.addClass('people-item-add')
      }

      $('button', elem).click(() => {
        this._removeLogin(login)
        this._renderList()
      })

      return elem
    }

    _renderList() {
      this.divList.empty()

      for(const login in this.data) {
        const stored = this.data[login]
        let row = this._createListRow(login, stored.staffData)
        row.appendTo(this.divList)
      }
      $('[data-login]').each(function(i, elem) {
        new StaffCard(this, $(this).attr('data-login'))
      })
    }

    _setupAutoComplete(input) {
      input.autocomplete({
        source: getStaffSuggest,
        minLength: 2,
        autoFocus: true,
        select: (event, ui) => {
          this._asyncAddLogin(ui.item.login, true)
          .then(() => {
            this._renderList()
          })
        },
        search: () => input.addClass('loading-gif'),
        open: () => input.removeClass('loading-gif')
      })
      .autocomplete( "instance" )._renderItem = function(ul, item) {
        const pic_src = `//center.yandex-team.ru/user/avatar/${item.login}/square`
        const li =
          `<li class="staff-item">
            <div>
              ${item.label} <br>
              <span class="depart">${item.department}</span>
            </div>
            <img src="${pic_src}">
          </li>`

        return $(li).appendTo(ul);
      };
    }

    _setInputMode(enable) {
      if (enable) {
        this._createInput().appendTo(this.divList)
        $('input', this.divList).show().focus()
      } else {
        $('input', this.divList).remove()
      }
    }

    _createInput() {
      let inp = $('<input class="form-control" id=staff-person-search autofocus type=text placeholder="имя или логин">')

      inp.focusout(()=> {
         this._setInputMode(false)
      })
      inp.keydown((event)=> {
        const ESC = 27
        if (event.which == ESC) {
          this._setInputMode(false)
        }
      })
      this._setupAutoComplete(inp)

      return inp
    }

    get_data() {
      let result = {
        unchanged: [],
        added: [],
        deleted: []
      }

      for (const login in this.data) {
        const loginData = this.data[login]
        if (loginData.justAdded) {
          result.added.push(login)
        } else if (loginData.deleted) {
          result.deleted.push(login)
        } else {
          result.unchanged.push(login)
        }
      }

      return result
    }

    show() {
      if (this.editMode) {
        $(this.addBtnId).click(() => {
          this._setInputMode(true)
        })
      }

      $(this.elemId).append(this.divList)
      this.ready // wait until staff data is ready
        .then(() => {
            this._renderList()
          },
          error => {
            this.divList.append('<div class="alert alert-danger" role="alert">Не удалось загрузить данные</div>')
            console.error(reason)
          }
        )
    }

    reset() {
      const logins = Object.keys(this.data)

      for (const login of logins) {
        if (this.data[login].justAdded) {
          delete(this.data[login])
        } else if (this.data[login].deleted) {
          this.data[login].deleted = false;
        }
      }
      this._renderList()
    }
  } // class PeopleView

  return {
    PeopleView: PeopleView
  }
}()

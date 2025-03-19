import './App.css'
import React from "react"
import axios from "axios"
import cookie from 'react-cookies';
import QueueList from "./components/queue_list"
import QueueForm from "./components/QueueForm";
import LoginForm from "./components/LoginForm";
import {HashRouter, Routes, Route} from "react-router-dom";

const endpoint = 'http://localhost:8000/api/filters/'
const endpoint_assignees = 'http://localhost:8000/api/s_units/'
const endpoint_assignees_abs = 'http://localhost:8000/api/s_units_absent/'
const endpoint_assignees_pe = 'http://localhost:8000/api/s_units_pending/'
const get_token_endpoint = 'http://localhost:8000/user/login/'


const NotFound404 = ({ location }) => {
return (
<div>
<h1>Page '{location.pathname}' not found</h1>
</div>
)
}

class App extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      'queues': [],
      'token': ''
    }
    this.get_headers.bind(this)
      this.is_authenticated.bind(this)
      this.deleteQueue.bind(this)
  }
  is_authenticated() {
        return !(this.state.token === '' || this.state.token === undefined);

    }

  set_token(token, username)
    {
        console.log('setting new token to state')
        this.setState({'token': token}, ()=>this.load_data())
        console.log('setting token to cookies')
        const cookies = cookie

        cookies.save('sk-token', token, {'maxAge': 3600})
        cookies.save('sk-login', username, {'maxAge': 3600})
        console.log(cookie.load('sk-login'))
    }

  get_token(username, password) {
        console.log('getting token via get_token function')
        axios.post(get_token_endpoint, {login: username, password: password})
            .then(response => {
                this.set_token(response.data.user['token'], username)
            })
            .catch(error => alert(error))

    }


  get_token_from_storage()
    {
        const cookies = cookie
        const token = cookies.load('sk-token')
        this.setState({'token': token}, ()=>this.load_data())
    }

  get_headers()
    {
        let headers = {
            'Content-Type': 'application/json'
        }

        if (this.is_authenticated())
        {
            headers['Authorization'] = 'Bearer ' + this.state.token
        }
        return headers
    }

  load_data() {

      axios.get(endpoint, {headers: this.get_headers()})
          .then(response => {

              const queues = response.data
              this.setState({'queues': queues}
            )

          }).catch(error => {
           if (error.response.status === 403)
                        {
                            console.log('403 catched: token outdated or no token present')
                            this.set_token('')
                        }
              console.log(error)
      })

      axios.get(endpoint_assignees, {headers: this.get_headers()})
          .then(response => {
              const s_units = response.data
              this.setState({'s_units': s_units})
          })
          .catch(error => {
           if (error.response.status === 403)
                        {
                            console.log('403 catched: token outdated or no token present')
                            this.set_token('')
                        }
              console.log(error)
      })

      axios.get(endpoint_assignees_abs, {headers: this.get_headers()})
          .then(response => {
              const s_units_abs = []
              response.data.map((absent) => {
                  s_units_abs.push(absent['login'])
                  return absent['login']
              })
              this.setState({'s_units_abs': s_units_abs})
          })
          .catch(error => {
           if (error.response.status === 403)
                        {
                            console.log('403 catched: token outdated or no token present')
                            this.set_token('')
                        }
              console.log(error)
      })

      axios.get(endpoint_assignees_pe, {headers: this.get_headers()})
          .then(response => {

              this.setState({'s_units_pending': response.data})
              })
          .catch(error => {
           if (error.response.status === 403)
                        {
                            console.log('403 catched: token outdated or no token present')
                            this.set_token('')
                        }
              console.log(error)
      })

  }

  componentDidMount() {
      this.load_data()
      this.get_token_from_storage()
  }

  editQueue(args) {
      let query_type = {false: 'query',
                        true: 'filter'}
        const headers = this.get_headers()
        const data = { 'q_id': args.q_id,
            'name': args.name,
            'ts_open': args.query_text,
            'crew_danger_limit': args.crew_danger_limit,
            'crew_warning_limit': args.crew_warning_limit,
            'ts_danger_limit': args.ts_danger_limit,
            'ts_warning_limit': args.ts_warning_limit,
            'query_type': query_type[args.is_filter],
            'assignees': args.assignees,
            'last_supervisor': cookie.load('sk-login'),
        }
        console.log(data.last_supervisor)
        axios.patch(endpoint + args.q_id + '/', data, {headers: headers})
            .then(response => {
                let new_queue = response.data
                console.log(new_queue)
            })
            .catch(error => console.log(error))

  }

  createQueue(args) {
      let query_type = {false: 'query',
                        true: 'filter'}
        const headers = this.get_headers()
        const data = { //'q_id': args.q_id,
            'name': args.queue_name,
            'ts_open': args.query_text,
            'crew_danger_limit': args.crew_danger_limit,
            'crew_warning_limit': args.crew_warning_limit,
            'ts_danger_limit': args.ts_danger_limit,
            'ts_warning_limit': args.ts_warning_limit,
            'query_type': query_type[args.is_filter],
            'assignees': args.assignees,
            'last_supervisor': cookie.load('sk-user'),
        }

        axios.post(endpoint, data, {headers: headers})
            .then(response => {
                let new_queue = response.data
                console.log(new_queue)
            })
            .catch(error => console.log(error))

  }

  deleteQueue(id) {
      let headers = this.get_headers()
      axios.delete(endpoint + id + '/', {headers: headers})
          .then(response => {
              this.setState({queues: this.state.queues.filter((item) => item.q_id !== id)})
              console.log(id)
          })
          .catch(error => console.log(error))

  }



    render() {
    return (
        <div className="App">
            <HashRouter>
                <Routes>
                    <Route exact path="/" element={<QueueList queues={this.state.queues}
                                                              deleteQueue={(id) => this.deleteQueue(id)}
                                                              units_absent = {this.state.s_units_abs}
                                                              units_pending={this.state.s_units_pending}
                                                              user_is_auth = {this.is_authenticated()}
                                                    />}
                    />
                    <Route exact path="/edit-assignees/:id" element={
                        <QueueForm queues={this.state.queues}
                                   callback={(args) => this.editQueue(args)}
                                   s_units={this.state.s_units}
                                   is_auth={this.is_authenticated()}
                        />}
                    />
                    <Route exact path="/new-queue/:id" element={
                        <QueueForm queues={this.state.queues}
                                   callback={(args) => this.createQueue(args)}
                                   s_units={this.state.s_units}
                                   is_auth={this.is_authenticated()}
                        />}
                    />

                    <Route component={NotFound404} />
                </Routes>
            </HashRouter>
            <LoginForm get_token={(email, password) => this.get_token(email, password)}
                        is_auth = {this.is_authenticated()}
            />
        </div>
    )
  }
}

export default App;

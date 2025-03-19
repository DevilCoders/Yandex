import React from 'react'
import cookie from "react-cookies";

class LoginForm extends React.Component {
    constructor(props) {
        super(props);
        this.state = {login:'', password: ''}
    }

    handleChange(event) {
        this.setState(
            {
                [event.target.name]: event.target.value
            }
        );
    }

    handleSubmit(event){
        this.props.get_token(this.state.login, this.state.password)
        event.preventDefault()
    }

    handleLogout(e){
        cookie.remove('sk-login')
        cookie.remove('sk-token')
        window.location.assign('/')
    }
    render(){
        if (this.props.is_auth === false) {
            return (
                <form onSubmit={(event) => this.handleSubmit(event)}
                      className="d-sm-inline-block form-inline mr-auto my-2 my-md-0 mw-70 ">
                    <input
                        className="form-control bg-light border-1 small"
                        type="text" name="login" placeholder="your-staff-login"
                        onChange={(event) => this.handleChange(event)}/>
                    <input
                        className="form-control bg-light border-1 small"
                        type="password" name="password" placeholder="Your Password here"
                        onChange={(event) => this.handleChange(event)}/>
                    <input
                        className="btn btn-primary d-none"
                        type="submit" value="Login"/>
                </form>

            )
        }
        else {
        return (
            <div>
                <p>{cookie.load('sk-login')}</p>
                <button className="btn btn-primary"
                onClick={(e)=>this.handleLogout(e)}
                >Logout</button>
            </div>
                )
        }
    }
}

export default LoginForm
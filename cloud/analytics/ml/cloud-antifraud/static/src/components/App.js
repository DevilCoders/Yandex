import React, { Component } from 'react';
import './App.css';
import Dash from './Charts/Dash';
import {BrowserRouter as Router, Route, Switch} from "react-router-dom";
import Explain from './Explain/Explain';
import Rules from './Rules/Rules';



class App extends Component {
  constructor(props) {
    super(props);
    this.state = {};
  }

  render() {
    return (
      <Router>
        <Switch>
            <Route exact path="/" component={Dash}/>
            <Route exact path="/explanation" component={Explain}/>
            <Route exact path="/rules" component={Rules} />
        </Switch>
      </Router>
    );
  }
}

export default App;

import React from 'react';
import ReactDOM from 'react-dom';

import './index.css';

import App from './components/App';
import * as serviceWorker from './serviceWorker';
import {Provider} from "react-redux";
import store from "./store";
import {configureRootTheme} from "@yandex-lego/components/Theme";
import {theme} from "@yandex-lego/components/Theme/presets/default";
import {BrowserRouter as Router} from "react-router-dom";

configureRootTheme({theme})


ReactDOM.render(
  <Provider store={store}>
      <Router>
          <App />
      </Router>
  </Provider>,
  document.getElementById('root')
);

// If you want your app to work offline and load faster, you can change
// unregister() to register() below. Note this comes with some pitfalls.
// Learn more about service workers: https://bit.ly/CRA-PWA
serviceWorker.unregister();

import {createStore, compose, applyMiddleware} from 'redux';
import reducer from 'app/reducers/index';
import thunkMiddleware from 'redux-thunk';
import {createLogger} from 'redux-logger';

let composeEnhancers = compose;

export function configureStore(state = {}) {
    if (typeof window !== 'undefined' && window.__REDUX_DEVTOOLS_EXTENSION__) {
        composeEnhancers = window.__REDUX_DEVTOOLS_EXTENSION_COMPOSE__;
    }
    if (process.env.NODE_ENV === 'local') {
        return createStore(reducer, state, composeEnhancers(applyMiddleware(thunkMiddleware, createLogger())));
    }
    return createStore(reducer, state, composeEnhancers(applyMiddleware(thunkMiddleware)));
}

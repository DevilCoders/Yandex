import './App.css';
import React from 'react';
import {BrowserRouter, Route, Switch, Redirect} from 'react-router-dom';
import TasksSummaryView from './views/TasksSummary';
import TasksSummaryView_v2 from './views/TasksSummary_v2';
import TaskView from './views/TaskView';
import {ui_prefix} from './Constants';


class App extends React.Component {
    render() {
        return (
            <BrowserRouter>
                <>
                    <Switch>
                        <Route
                            path={`${ui_prefix}/tasks/v2`}
                            component={TasksSummaryView_v2}
                        />
                        <Route
                            path={`${ui_prefix}/tasks`}
                            component={TasksSummaryView}
                        />
                        <Route
                            path={`${ui_prefix}/task/:task_name`}
                            component={TaskView}
                        />
                        <Redirect to={`${ui_prefix}/tasks/v2`}/>
                    </Switch>
                </>
            </BrowserRouter>
        )
    }
}

export default App;

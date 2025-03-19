import React from 'react';
import {
    Route,
    Switch
} from 'react-router-dom'

import './App.css';

import {
    Home,
    JobResults,
    Masters,
    Minions,
    Shipment,
    Shipments,

    Page,
} from "../containers";

import Footer from "./Footer"
import Header from "./Header"


const App: React.FC = () => {
    return (
        <div className="App">
            <Header/>
            <hr/>
                <Switch>
                    <Route path="/masters">
                        <Page title="Masters">
                            <Masters/>
                        </Page>
                    </Route>
                    <Route path="/minions">
                        <Page title="Minions">
                            <Minions/>
                        </Page>
                    </Route>
                    <Route path="/shipments/:id">
                        <Page title="Shipment">
                            <Shipment/>
                        </Page>
                    </Route>
                    <Route path="/shipments">
                        <Page title="Shipments">
                            <Shipments/>
                        </Page>
                    </Route>
                    <Route path="/jobresults">
                        <Page title="JobResults">
                            <JobResults/>
                        </Page>
                    </Route>
                    <Route path="/">
                        <Page title="Home">
                            <Home/>
                        </Page>
                    </Route>
                </Switch>
            <Footer/>
        </div>
    );
}

export default App;

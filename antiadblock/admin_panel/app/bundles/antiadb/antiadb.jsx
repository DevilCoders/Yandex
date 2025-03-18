import 'lego-on-react/src/components/logo/logo.css';
import 'lego-on-react/src/components/header/header.css';
import 'lego-on-react/src/components/textinput/textinput.css';
import 'lego-on-react/src/components/textarea/textarea.css';
import 'lego-on-react/src/components/spin/spin.css';
import 'lego-on-react/src/components/link/link.css';
import 'lego-on-react/src/components/button/button.css';
import 'lego-on-react/src/components/checkbox/checkbox.css';
import 'lego-on-react/src/components/radio-button/radio-button.css';
import 'lego-on-react/src/components/modal/modal.css';
import 'lego-on-react/src/components/tooltip/tooltip.css';
import 'lego-on-react/src/components/icon/icon.css';
import 'lego-on-react/src/components/user/user.css';
import 'lego-on-react/src/components/user-account/user-account.css';
import 'lego-on-react/src/components/checkbox/checkbox.css';
import 'lego-on-react/src/components/tumbler/tumbler.css';
import 'lego-on-react/src/components/select/select.css';
import 'lego-on-react/src/components/footer/footer.css';

import 'app/components/modal/modal.css';
import 'app/components/modal/__title/modal__title.css';
import 'app/components/modal/__body/modal__body.css';
import 'app/components/modal/__actions/modal__actions.css';

import React, {Component} from 'react';
import PropTypes from 'prop-types';
import {Switch, Route} from 'react-router-dom';
import Helmet from 'react-helmet';

import {configureStore} from 'app/redux/store';
import {getInitialState} from 'app/redux/state';

import Page from 'app/components/page/page';
import Error from 'app/components/error/error';

import {setUser} from 'app/lib/user';
import {setPermissions} from 'app/lib/permissions';

import 'app/components/b-page/b-page.css';

class App extends Component {
    getChildContext() {
        return {
            lang: this.props.appData.lang,
            host: this.props.appData.host,
            tld: this.props.appData.tld
        };
    }

    constructor(props) {
        super();

        setUser({
            ...props.appData.user,
            yandexUid: props.appData.yandexuid
        });
        setPermissions(props.appData.permissions);
    }

    render() {
        return (
            <div className='app'>
                <Helmet title='Antiblock' />
                <Switch>
                    <Route path='/' exact component={Page} />
                    <Route path='/support' exact component={Page} />
                    <Route path='/search' exact component={Page} />
                    <Route path='/service/:serviceId?' component={Page} />
                    <Route path='/state/:deviceType?' exact component={Page} />
                    <Route path='/heatmap' exact component={Page} />
                    <Route path='/logs/*' exact component={Page} />
                    <Route path='*' component={Error} />
                </Switch>
            </div>
        );
    }
}

App.propTypes = {
    appData: PropTypes.object
};

App.childContextTypes = {
    lang: PropTypes.string,
    host: PropTypes.string,
    tld: PropTypes.string
};

App.getInitialState = getInitialState;
App.configureStore = configureStore;

export default App;

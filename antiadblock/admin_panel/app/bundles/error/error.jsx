import 'lego-on-react/src/components/logo/logo.css';
import 'lego-on-react/src/components/header/header.css';
import 'lego-on-react/src/components/user/user.css';
import 'lego-on-react/src/components/user-account/user-account.css';
import 'lego-on-react/src/components/link/link.css';
import 'lego-on-react/src/components/select/select.css';
import 'lego-on-react/src/components/footer/footer.css';
import 'lego-on-react/src/components/textinput/textinput.css';
import 'lego-on-react/src/components/icon/icon.css';
import 'lego-on-react/src/components/tooltip/tooltip.css';

import React, {Component} from 'react';
import PropTypes from 'prop-types';
import Helmet from 'react-helmet';

import Error from 'app/components/error/error';

import {configureStore} from 'app/redux/store';
import {getInitialState} from 'app/redux/state';

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
                <Error
                    status={this.props.appData.error.status}
                    messageI18N={this.props.appData.error.messageI18N} />
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

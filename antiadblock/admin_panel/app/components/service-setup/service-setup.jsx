import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import {getService} from 'app/reducers/index';
import {fetchServiceSetup} from 'app/actions/service';

import Bem from 'app/components/bem/bem';
import Preloader from 'app/components/preloader/preloader';
import Hint from 'app/components/hint/hint';
import Textarea from 'lego-on-react/src/components/textarea/textarea.react';
import Button from 'lego-on-react/src/components/button/button.react';
import Link from 'lego-on-react/src/components/link/link.react';

import {setupType} from 'app/types';

import './service-setup.css';

import i18n from 'app/lib/i18n';

const FIELDS = {
    TOKEN: 'token',
    JS_INLINE: 'js_inline',
    NGINX_CONFIG: 'nginx_config'
};

const DEBOUNCE_WAIT = 2000;

class ServiceSetup extends React.Component {
    constructor() {
        super();

        this._scope = {};
        this.setWrapperRef = this.setWrapperRef.bind(this);
        this.state = {
            copiedLinks: {}
        };

        this._textAreas = {};
        this._timeouts = {};
    }

    componentDidMount() {
        this.props.fetchServiceSetup(this.props.serviceId);
    }

    setWrapperRef(wrapper) {
        this._scope.dom = wrapper;
    }

    componentWillUnmount() {
        Object.values(this._timeouts).forEach(timeout => clearTimeout(timeout));
    }

    resetCopyButton(areaName) {
        this._timeouts[areaName] = setTimeout(() => {
            this.setState(state => ({
                copiedLinks: {
                    ...state.copiedLinks,
                    [areaName]: false
                }
            }));
        }, DEBOUNCE_WAIT);
    }

    onCopyButtonClick(areaName, event) {
        event.preventDefault();

        this._textAreas[areaName]._control.select();
        document.execCommand('copy');

        this.setState(state => ({
            copiedLinks: {
                ...state.copiedLinks,
                [areaName]: true
            }
        }));
        this.resetCopyButton(areaName);
    }

    setTextAreaRef(areaName, textAreaRef) {
        this._textAreas[areaName] = textAreaRef;
    }

    onFocusTextArea(areaName) {
        this._textAreas[areaName]._control.select();
    }

    render() {
        return (
            <Bem
                tagRef={this.setWrapperRef}
                block='service-setup'>
                {!this.props.setup.loaded ?
                    <Preloader /> :
                    [
                        <Bem
                            key='tech'
                            block='service-setup'
                            elem='tech'>
                            {i18n('service-page', 'tech-link-before')}&nbsp;
                            <Link
                                tone='grey'
                                view='default'
                                theme='link'
                                type='link'
                                url={i18n('service-page', 'tech-link')}>
                                {i18n('service-page', 'tech-link-text')}.
                            </Link>
                        </Bem>,
                        ...Object.values(FIELDS).map(field => (
                            <Bem
                                key={field}
                                block='service-setup'
                                elem='field'>
                                <Bem
                                    key='label'
                                    block='service-setup'
                                    elem='label'>
                                    {i18n('service-page', `service-setup-${field}`)}
                                    <Hint
                                        text={i18n('service-page', `service-setup-${field}-hint`)}
                                        scope={this._scope} />
                                </Bem>
                                <Bem
                                    key='value'
                                    block='service-setup'
                                    elem='value'>
                                    <Textarea
                                        onFocus={this.onFocusTextArea.bind(this, field)} // eslint-disable-line react/jsx-no-bind
                                        ref={this.setTextAreaRef.bind(this, field)} // eslint-disable-line react/jsx-no-bind
                                        theme='normal'
                                        tone='grey'
                                        view='default'
                                        size='s'
                                        text={this.props.setup[field]} />
                                </Bem>
                                <Bem
                                    key='copy-button'
                                    block='service-setup'
                                    elem='copy-button'>
                                    {!this.state.copiedLinks[field] ?
                                        <Button
                                            onClick={this.onCopyButtonClick.bind(this, field)} // eslint-disable-line react/jsx-no-bind
                                            view='default'
                                            tone='grey'
                                            size='s'
                                            theme='normal' >
                                            {i18n('common', 'copy')}
                                        </Button> :
                                        <Bem
                                            block='service-setup'
                                            elem='copy-link-success'>
                                            {i18n('service-page', 'text-was-copied')}
                                        </Bem>}
                                </Bem>
                            </Bem>
                        ))
                    ]
                }
            </Bem>
        );
    }
}

ServiceSetup.propTypes = {
    serviceId: PropTypes.string.isRequired,
    setup: setupType.isRequired,
    loaded: PropTypes.bool.isRequired,
    fetchServiceSetup: PropTypes.func.isRequired
};

export default connect(state => {
    return {
        serviceId: getService(state).service.id,
        setup: getService(state).setup.data,
        loaded: getService(state).setup.loaded
    };
}, dispatch => {
    return {
        fetchServiceSetup: serviceId => {
            return dispatch(fetchServiceSetup(serviceId));
        }
    };
})(ServiceSetup);

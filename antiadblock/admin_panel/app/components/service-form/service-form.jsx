import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';
import {Link} from 'react-router-dom';

import {setGlobalErrors} from 'app/actions/errors';

import Bem from 'app/components/bem/bem';
import Textinput from 'lego-on-react/src/components/textinput/textinput.react';
import Button from 'lego-on-react/src/components/button/button.react';
import Tooltip from 'lego-on-react/src/components/tooltip/tooltip.react';

import servicesApi from 'app/api/service';
import redirect from 'app/lib/redirect';
import {antiadbUrl} from 'app/lib/url';

import i18n from 'app/lib/i18n';

import './service-form.css';

const FIELDS = [
    'service_id',
    'name',
    'domain'
];

class ServiceForm extends React.Component {
    constructor() {
        super();

        this._scope = {};
        this._anchors = {};

        this.state = {
            ...FIELDS.reduce((map, field) => {
                map[field] = '';

                return map;
            }, {}),
            saving: false,
            errors: {}
        };

        this.setWrapperRef = this.setWrapperRef.bind(this);
        this.onSave = this.onSave.bind(this);
    }

    setWrapperRef(wrapper) {
        this._scope.dom = wrapper;
    }

    getAnchorRefSetter(name) {
        return ref => {
            this._anchors[name] = ref;
        };
    }

    getOnChangeHandler(fieldName) {
        return value => {
            this.setState(state => ({
                [fieldName]: value,
                errors: {
                    ...state.errors,
                    [fieldName]: null
                }
            }));
        };
    }

    onSave() {
        this.setState({
            saving: true
        });

        servicesApi.createService(this.state.service_id, this.state.name, this.state.domain)
            .then(service => {
                redirect(antiadbUrl(`/service/${service.id}`));
            }, error => {
                if (error.response.properties) {
                    this.setErrors(error.response.properties);
                } else {
                    this.props.setGlobalErrors([error.message]);
                }
            });
    }

    setErrors(properties) {
        this.setState({
            saving: false,
            errors: {
                ...properties.reduce((map, property) => {
                    map[property.path[0]] = property.message;

                    return map;
                }, {})
            }
        });
    }

    render() {
        return (
            <Bem
                tagRef={this.setWrapperRef}
                block='service-form'>
                <Bem
                    key='head'
                    block='service-form'
                    elem='head'>
                    {i18n('service-page', 'service-form-title')}
                </Bem>
                <Bem
                    key='body'
                    block='service-form'
                    elem='body'>
                    {FIELDS.map(field => (
                        <Bem
                            key={`${field}`}
                            block='service-form'
                            elem='block'>
                            <Bem
                                key='label'
                                block='service-form'
                                elem='label'>
                                {i18n('service-page', `service-${field}`)}
                            </Bem>
                            <Bem
                                key='input'
                                block='service-form'
                                elem='input'
                                ref={this.getAnchorRefSetter(field)}>
                                <Textinput
                                    theme='normal'
                                    tone='grey'
                                    view='default'
                                    size='m'
                                    text={this.state[field]}
                                    placeholder={i18n('service-page', `service-${field}-placeholder`)}
                                    onChange={this.getOnChangeHandler(field)} />
                            </Bem>
                            <Tooltip
                                anchor={this._anchors[field]}
                                view='classic'
                                tone='default'
                                theme='error'
                                mix={{
                                    block: 'service-form',
                                    elem: 'tooltip'
                                }}
                                mainOffset={10}
                                visible={Boolean(this.state.errors[field])}
                                scope={this._scope.dom}>
                                {this.state.errors[field]}
                            </Tooltip>
                        </Bem>
                    ))}
                </Bem>
                <Bem
                    key='actions'
                    block='service-form'
                    elem='actions'>
                    <Bem
                        key='cancel'
                        block='service-form'
                        elem='button'>
                        <Link
                            to={antiadbUrl('/')}>
                            <Button
                                view='default'
                                tone='grey'
                                size='n'
                                theme='normal'>
                                {i18n('common', 'cancel')}
                            </Button>
                        </Link>
                    </Bem>
                    <Bem
                        key='add'
                        block='service-form'
                        elem='button'>
                        <Button
                            view='default'
                            tone='grey'
                            type='action'
                            size='n'
                            theme='action'
                            progress={this.state.saving}
                            onClick={this.onSave}>
                            {i18n('common', 'add')}
                        </Button>
                    </Bem>
                </Bem>
            </Bem>
        );
    }
}

ServiceForm.propTypes = {
    setGlobalErrors: PropTypes.func
};

export default connect(null, dispatch => {
    return {
        setGlobalErrors: errors => {
            return dispatch(setGlobalErrors(errors));
        }
    };
})(ServiceForm);

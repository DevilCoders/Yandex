import React from 'react';
import PropTypes from 'prop-types';
import queryString from 'query-string';

import {antiadbUrl} from 'app/lib/url';
import redirect from 'app/lib/redirect';

import TextInput from 'lego-on-react/src/components/textinput/textinput.react';
import Tooltip from 'lego-on-react/src/components/tooltip/tooltip.react';

import Bem from 'app/components/bem/bem';
import Hint from 'app/components/hint/hint';

import i18n from 'app/lib/i18n';
import servicesApi from 'app/api/services';

import {KEYS} from 'app/enums/keys';

import './search-input.css';

export default class SearchInput extends React.Component {
    constructor(props) {
        super(props);

        const parsed = queryString.parse(this.props.pattern);

        this.state = {
            pattern: parsed.pattern || '',
            errors: ''
        };

        this.onChangePattern = this.onChangePattern.bind(this);
        this.onSearchEnter = this.onSearchEnter.bind(this);
        this.setIconRef = this.setIconRef.bind(this);
    }

    setIconRef(icon) {
        this._icon = icon;
    }

    setErrors(error) {
        this.setState({
            errors: error
        });
    }

    onChangePattern(value) {
        this.setState({
            pattern: value,
            errors: ''
        });
    }

    onSearchEnter(event) {
        if (event.keyCode === KEYS.ENTER && this.state.pattern) {
            const parsed = queryString.parse(location.search);

            const active = parsed.active === 'true' || parsed.active === undefined;
            servicesApi.searchConfigs(this.state.pattern, 0, 20, active)
                .then(() => {
                    redirect(antiadbUrl(`/search?pattern=${encodeURIComponent(this.state.pattern)}&active=${active}`));
                }, result => {
                    if (result.status === 400) {
                        const response = result.response;
                        this.setErrors(response.properties[0].message);
                    }
                });
        }
    }

    render() {
        return (
            <Bem
                block='search-input'
                mods={{
                    'has-errors': Boolean(this.state.errors)
                }}>
                <Bem
                    block='search-input'
                    elem='search'
                    ref={this.setIconRef}>
                    <TextInput
                        theme='normal'
                        tone='grey'
                        view='default'
                        size='m'
                        hasClear
                        text={this.state.pattern}
                        placeholder={i18n('common', 'search-configs')}
                        onChange={this.onChangePattern}
                        onKeyDown={this.onSearchEnter} />
                </Bem>
                <Tooltip
                    anchor={this._icon}
                    view='classic'
                    tone='default'
                    theme='error'
                    to={['right-center']}
                    mainOffset={10}
                    zIndexGroupLevel={100}
                    mix={{
                        block: 'modal',
                        elem: 'tooltip'
                    }}
                    visible={Boolean(this.state.errors)}>
                    {this.state.errors}
                </Tooltip>
                <Bem
                    block='search-input'
                    elem='hint'>
                    <Hint
                        text={i18n('search-page', 'search-input-hint')}
                        to='bottom' />
                </Bem>
            </Bem>
        );
    }
}

SearchInput.propTypes = {
    pattern: PropTypes.string
};

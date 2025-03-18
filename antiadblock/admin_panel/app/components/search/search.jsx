import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';
import queryString from 'query-string';

import {searchConfigs} from 'app/actions/search';
import {getSearch} from 'app/reducers/index';

import Tumbler from 'app/components/tumbler/tumbler';

import Preloader from 'app/components/preloader/preloader';
import Bem from 'app/components/bem/bem';
import InfiniteList from 'app/components/infinite-list/infinite-list';
import SearchConfigsItem from './__item/search__item';

import ConfigDiffModal from 'app/components/config-diff-modal/config-diff-modal';
import ConfigPreview from 'app/components/config-preview/config-preview';

import Hint from 'app/components/hint/hint';

import i18n from 'app/lib/i18n';

import './search.css';
import {antiadbUrl} from 'app/lib/url';

class SearchPage extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            pattern: this.props.pattern,
            active: this.props.tumblerState
        };
        this.onChange = this.onChange.bind(this);
    }

    onChange() {
        this.setState(state => ({
            active: !state.active
        }));
        this.context.router.history.push(antiadbUrl(`/search?pattern=${this.state.pattern.replace(/\+/g, '%2B')}&active=${!this.state.active}`));
    }

    render() {
        return (
            <Bem
                block='search'>
                {!this.props.loaded ?
                    <Preloader key='preloader' /> :
                    ''}
                <Bem
                    block='search'
                    elem='count'>
                    {i18n('search-page', 'total')}: {this.props.total}
                </Bem>
                <Bem
                    block='search'
                    elem='active-before'
                    key='active-before'>
                    {i18n('search-page', 'active')}
                </Bem>
                <Bem
                    block='search'
                    elem='hint'>
                    <Hint
                        text={i18n('search-page', 'active-hint')}
                        to='right' />
                </Bem>
                <Bem
                    block='search'
                    elem='active'
                    key='active'>
                    <Tumbler
                        theme='normal'
                        view='default'
                        tone='grey'
                        size='s'
                        onVal='onVal'
                        offVal='offVal'
                        checked={this.state.active}
                        onChange={this.onChange} />
                </Bem>
                <Bem
                    key='wrapper'
                    block='search'
                    elem='wrapper'>
                    <InfiniteList
                        key='list'
                        wrapperMix={{
                            block: 'search',
                            elem: 'scroll-wrapper'
                        }}
                        filters={{
                            active: this.state.active
                        }}
                        requestMore={this.props.searchConfigs}
                        total={this.props.total}
                        items={this.props.configs}
                        Item={SearchConfigsItem} />
                </Bem>
                <ConfigPreview
                    key='preview' />
                <ConfigDiffModal
                    key='diff' />
            </Bem>
        );
    }
}

SearchPage.propTypes = {
    configs: PropTypes.array,
    loaded: PropTypes.bool,
    total: PropTypes.number,
    tumblerState: PropTypes.bool,
    pattern: PropTypes.string,
    searchConfigs: PropTypes.func
};

SearchPage.contextTypes = {
    router: {
        history: {
            push: PropTypes.func.isRequired
        }
    }
};

export default connect((state, props) => {
    const parsed = queryString.parse(props.location.search);
    const configs = getSearch(state);
    return {
        configs: configs.items,
        loaded: configs.loaded,
        total: configs.total,
        pattern: parsed.pattern,
        tumblerState: parsed.active === 'true'
    };
}, (dispatch, props) => {
    return {
        searchConfigs: (offset, limit, filters) => {
            const parsed = queryString.parse(props.location.search);
            return dispatch(searchConfigs(parsed.pattern, offset, limit, filters.active));
        }
    };
})(SearchPage);

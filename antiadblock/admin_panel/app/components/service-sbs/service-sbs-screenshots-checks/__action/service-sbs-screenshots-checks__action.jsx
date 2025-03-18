import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Select from 'lego-on-react/src/components/select/select.react';
import Checkbox from 'lego-on-react/src/components/checkbox/checkbox.react';
import Link from 'lego-on-react/src/components/link/link.react';

import i18n from 'app/lib/i18n';

import './service-sbs-screenshots-checks__action.css';

const MAX_SELECT_LENGTH_VALUE = 150;

class ServiceSbsScreenshotsChecksAction extends React.Component {
    constructor(props) {
        super(props);

        this.onChangeFilter = this.onChangeFilter.bind(this);
        this.onChangeErrorCases = this.onChangeErrorCases.bind(this);
    }

    prepareSelectValue(value) {
        return value[0];
    }

    onChangeFilter(key) {
        return val => {
            this.props.onChangeFilter(key, this.prepareSelectValue(val));
        };
    }

    onChangeErrorCases() {
        this.props.onChangeFilter('byErrorCases', !this.props.filters.byErrorCases);
    }

    getShortValue(value) {
        const val = value.toString();

        if (val.length > MAX_SELECT_LENGTH_VALUE) {
            return val.slice(0, MAX_SELECT_LENGTH_VALUE) + '...';
        }

        return val;
    }

    renderActionBlock(options, direction, fixSize = false) {
        const {
            data,
            filterKey,
            title
        } = options;

        return (
            <Bem
                key={title}
                block='service-sbs-screenshots-checks-action'
                elem='block'>
                {direction !== 'Right' ?
                    <Bem
                        block='service-sbs-screenshots-checks-action'
                        elem='block-title'>
                        {i18n('sbs', title)}
                    </Bem> : null}
                <Bem
                    block='service-sbs-screenshots-checks-action'
                    elem='block-select'
                    mods={{
                        'fix-size': fixSize
                    }}>
                    <Select
                        theme='pseudo'
                        tone='grey'
                        size='s'
                        type='radio'
                        width='max'
                        val={this.props.filters[filterKey]}
                        onChange={this.onChangeFilter(filterKey)}
                        popup={{
                            scope: this.context.scope && this.context.scope.dom
                        }}>
                        {data.map(value => {
                            return (
                                <Select.Item
                                    key={value}
                                    val={value} >
                                    {i18n('sbs', this.getShortValue(value))}
                                </Select.Item>
                            );
                        })}
                    </Select>
                </Bem>
            </Bem>
        );
    }

    renderCmpActionBlock(flag) {
        const data = [{
            data: this.props.enums.blockers,
            filterKey: 'byBlocker' + flag,
            title: 'blocker'
        }, {
            data: this.props.enums.runIds,
            filterKey: 'byRunId' + flag,
            title: 'runId'
        }];

        return (
            [data.map(item => (
                this.renderActionBlock(item, flag, true)
            ))]
        );
    }

    renderCmpActions() {
        return (
            <Bem
                block='service-sbs-screenshots-checks-action'
                elem='cmp'>
                <Bem
                    block='service-sbs-screenshots-checks-action'
                    elem='cmp-left'>
                    {this.renderCmpActionBlock('Left')}
                </Bem>
                <Bem
                    block='service-sbs-screenshots-checks-action'
                    elem='cmp-right'>
                    {this.renderCmpActionBlock('Right')}
                </Bem>
            </Bem>
        );
    }

    renderGeneralActionsTop() {
        const data = [{
            data: this.props.enums.urls,
            filterKey: 'byUrl',
            title: 'url'
        }, {
            data: this.props.enums.browsers,
            filterKey: 'byBrowser',
            title: 'browser'
        }];

        return (
            <Bem
                block='service-sbs-screenshots-checks-action'
                elem='general-top'>
                { data.map(item => this.renderActionBlock(item)) }
            </Bem>
        );
    }

    renderGeneralActionsBottom() {
        return (
            <Bem
                block='service-sbs-screenshots-checks-action'
                elem='general-bottom'>
                <Bem
                    block='service-sbs-screenshots-checks-action'
                    elem='general-bottom-item'>
                    <Checkbox
                        theme='normal'
                        size='s'
                        tone='grey'
                        view='default'
                        checked={this.props.filters.byErrorCases}
                        onChange={this.onChangeErrorCases}>
                        {i18n('sbs', 'show-only-error-cases')}
                    </Checkbox>
                    <Bem
                        block='service-sbs-screenshots-checks-action'
                        elem='general-bottom-item-clear'>
                        <Link
                            theme='pseudo'
                            onClick={this.props.onResetFilters}>
                            {i18n('sbs', 'clear-filter')}
                        </Link>
                    </Bem>
                </Bem>
            </Bem>
        );
    }

    render() {
        return (
            <Bem
                block='service-sbs-screenshots-checks-action'>
                {this.renderGeneralActionsTop()}
                {this.renderCmpActions()}
                {this.renderGeneralActionsBottom()}
            </Bem>
        );
    }
}

ServiceSbsScreenshotsChecksAction.propTypes = {
    onChangeFilter: PropTypes.func.isRequired,
    onResetFilters: PropTypes.func.isRequired,
    filters: PropTypes.object.isRequired,
    enums: PropTypes.object.isRequired
};

export default ServiceSbsScreenshotsChecksAction;

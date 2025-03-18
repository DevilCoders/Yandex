import React from 'react';
import PropTypes from 'prop-types';
import Bem from 'app/components/bem/bem';

import Preloader from 'app/components/preloader/preloader';
import Spin from 'lego-on-react/src/components/spin/spin.react';

import isEqual from 'lodash/isEqual';
import List from './__list/infinite-list__list';

import './infinite-list.css';

const DEFAULT_LIMIT = 20;
const COEFFICIENT_FOR_LIMIT = 45;

export default class InfiniteList extends React.Component {
    constructor(props) {
        super(props);
        this.scope = {};
        this.state = {
            initiallyLoaded: false,
            isLoadingMore: false
        };

        this.setWrapperRef = this.setWrapperRef.bind(this);
        this.setScopeRef = this.setScopeRef.bind(this);
        this.onScroll = this.onScroll.bind(this);
        this.requestInitially = this.requestInitially.bind(this);
        const body = document.querySelector('body').getBoundingClientRect();
        this.computedLimit = Math.max(
            Math.ceil(body.height / COEFFICIENT_FOR_LIMIT),
            DEFAULT_LIMIT,
        );
    }

    requestInitially() {
        this.props.requestMore(0, this.computedLimit, this.props.filters).then(() => {
            this.setState({
                initiallyLoaded: true
            });
        });
    }

    componentDidMount() {
        if (!this.state.initiallyLoaded) {
            this.requestInitially();
        }

        const scope = this.context.scope;
        if (scope && scope.dom) {
            scope.dom.addEventListener('scroll', this.onScroll);
        }
    }

    componentDidUpdate() {
        const scope = this.context.scope;
        if (scope && scope.dom) {
            scope.dom.addEventListener('scroll', this.onScroll);
        }
    }

    componentWillUnmount() {
        const scope = this.context.scope;
        if (scope && scope.dom) {
            scope.dom.removeEventListener('scroll', this.onScroll);
        }
    }

    componentWillReceiveProps(newProps) {
        if (!isEqual(newProps.filters, this.props.filters)) {
            this.setState({
                initiallyLoaded: false
            }, () => {
                this.requestInitially();
            });
        } else if (newProps.items && newProps.items.length !== this.props.items.length) {
            this.setState({
                isLoadingMore: false
            });
        }
    }

    onScroll() {
        const wrapper = this._wrapper;
        const hasMoreItems = this.props.items.length < this.props.total;
        const height = isNaN(window.innerHeight) ? window.clientHeight : window.innerHeight;
        const rect = wrapper.getBoundingClientRect();
        const isAtBottom = rect.bottom < height;

        if (isAtBottom && !this.state.isLoadingMore && hasMoreItems) {
            this.props.requestMore(this.props.items.length, this.computedLimit, this.props.filters);
            this.setState({
                isLoadingMore: true
            });
        }
    }

    setWrapperRef(wrapper) {
        this._wrapper = wrapper;
    }

    setScopeRef(list) {
        this.scope.dom = list;
    }

    shouldComponentUpdate(nextProps, nextState) {
        return this.state.isLoadingMore !== nextState.isLoadingMore ||
            nextProps.items.length !== this.props.items.length ||
            nextState.initiallyLoaded !== this.state.initiallyLoaded;
    }

    render() {
        const listIsEmpty = this.state.initiallyLoaded && this.props.items.length === 0;
        const shouldShowEmptyLabel = listIsEmpty && this.props.emptyLabel;

        return (
            <Bem
                block='infinite-list'
                mix={this.props.mix}>
                {[
                    !this.state.initiallyLoaded ? <Preloader key='preloader' /> : null,
                    <Bem
                        key='wrapper'
                        block='infinite-list'
                        elem='wrapper'
                        mix={this.props.wrapperMix}
                        tagRef={this.setWrapperRef}>
                        {shouldShowEmptyLabel ?
                            this.props.emptyLabel :
                            (
                                <List
                                    key='content'
                                    ref={this.setScopeRef}
                                    items={this.props.items}
                                    Item={this.props.Item}
                                    scope={this.scope}>
                                    {this.props.children}
                                </List>
                            )
                        }
                        <Bem
                            block='infinite-list'
                            elem='scroll-spin'>
                            <Spin
                                size='s'
                                progress={this.state.isLoadingMore} />
                        </Bem>
                    </Bem>
                ]}
            </Bem>
        );
    }
}

InfiniteList.contextTypes = {
    scope: PropTypes.object
};

InfiniteList.propTypes = {
    requestMore: PropTypes.func.isRequired,
    items: PropTypes.array.isRequired,

    mix: PropTypes.object,
    wrapperMix: PropTypes.object,
    children: PropTypes.array,

    emptyLabel: PropTypes.string,

    total: PropTypes.number,
    filters: PropTypes.object,

    Item: PropTypes.func.isRequired
};

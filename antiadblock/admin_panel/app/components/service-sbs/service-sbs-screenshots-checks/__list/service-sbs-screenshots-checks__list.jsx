import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';

import i18n from 'app/lib/i18n';
import isEqual from 'lodash/isEqual';
import {sbsRunIdCasesType} from 'app/types';
import ServiceSbsScreenshotsChecksItem from '../__item/service-sbs-screenshots-checks__item';

import './service-sbs-screenshots-checks__list.css';
import 'app/components/icon/_theme/icon_theme_link-external.css';

const SCROLL_PERCENT = 10;

export default class ServiceSbsScreenshotsChecksList extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            countVisible: 0,
            step: 0,
            pairs: []
        };

        this.scrollList = this.scrollList.bind(this);
    }

    componentDidMount() {
        const pairs = this.getPairs(this.props);
        let step = pairs.length;

        const scope = this.context.scope;
        if (scope && scope.dom) {
            scope.dom.addEventListener('scroll', this.scrollList);
            step = this.calculatePercentage(pairs.length, SCROLL_PERCENT);
        }

        this.setState({
            pairs,
            countVisible: step,
            step
        });
    }

    componentWillUnmount() {
        const scope = this.context.scope;
        if (scope && scope.dom) {
            scope.dom.removeEventListener('scroll', this.scrollList);
        }
    }

    componentWillReceiveProps(nextProps) {
        if (!isEqual(nextProps.casesLeft, this.props.casesLeft) || !isEqual(nextProps.casesRight, this.props.casesRight) ||
            !isEqual(nextProps.leftRunId, this.props.leftRunId) || !isEqual(nextProps.rightRunId, this.props.rightRunId) ||
            this.props.onlyErrorCases !== nextProps.onlyErrorCases) {
            const pairs = this.getPairs(nextProps);
            let step = pairs.length;

            const scope = this.context.scope;
            if (scope && scope.dom) {
                step = this.calculatePercentage(pairs.length, SCROLL_PERCENT);
            }
            this.setState({
                pairs,
                countVisible: step,
                step
            });
        }
    }

    calculatePercentage(len, percent) {
        return Math.ceil(len * percent / 100);
    }

    scrollList() {
        const scope = this.context.scope.dom;
        if (scope.scrollTop + scope.offsetHeight >= scope.scrollHeight - scope.offsetHeight) {
            this.setState(state => ({
                countVisible: state.countVisible + state.step
            }));
        }
    }

    filterPairs(isEq) {
        const notEqCb = (_, item1, item2) => item1.browser === item2.browser && item1.url === item2.url;
        const eqCb = (pairs, item1, item2) =>
            notEqCb(pairs, item1, item2) &&
            (item1.adblocker !== item2.adblocker || item1.browser !== item2.browser) &&
            !pairs.some(item => (isEqual(item.left, item2) && isEqual(item.right, item1)));

        return isEq ? eqCb : notEqCb;
    }

    isProblem(left, right) {
        const IS_PROBLEM = 'problem';

        return left.has_problem === IS_PROBLEM || right.has_problem === IS_PROBLEM;
    }

    getPairs(props) {
        const {
            leftRunId,
            rightRunId,
            casesLeft,
            casesRight
        } = props;
        const filterPairs = this.filterPairs(leftRunId === rightRunId);

        return casesLeft.reduce((pairs, item1) => {
            casesRight.forEach(item2 => {
                if ((!props.onlyErrorCases || this.isProblem(item1, item2)) && filterPairs(pairs, item1, item2)) {
                    pairs.push({
                        left: item1,
                        right: item2
                    });
                }
            });

            return pairs;
        }, []);
    }

    render() {
        const pairs = this.state.pairs.slice(0, this.state.countVisible + 1);

        return (
            <Bem
                block='service-sbs-screenshots-checks-list'>
                {pairs.length ?
                    <Bem
                        block='service-sbs-screenshots-checks-list'
                        elem='body'>
                        {pairs.map((item, index) => (
                            <ServiceSbsScreenshotsChecksItem
                                /* eslint-disable-next-line react/no-array-index-key */
                                key={`service-sbs-screenshots-checks-list-item-${index}`}
                                pair={item}
                                onChangeModalVisible={this.props.onChangeModalVisible}
                                onChangeFilter={this.props.onChangeFilter} />
                            ))}
                    </Bem> :
                    <Bem
                        block='service-sbs-screenshots-checks-list'
                        elem='empty'>
                        {i18n('sbs', 'not-found')}
                    </Bem>}
            </Bem>
        );
    }
}

ServiceSbsScreenshotsChecksList.contextTypes = {
    scope: PropTypes.object
};

ServiceSbsScreenshotsChecksList.propTypes = {
    leftRunId: PropTypes.number.isRequired,
    rightRunId: PropTypes.number.isRequired,
    casesLeft: sbsRunIdCasesType.isRequired,
    casesRight: sbsRunIdCasesType.isRequired,
    onChangeFilter: PropTypes.func.isRequired,
    onChangeModalVisible: PropTypes.func.isRequired,
    onlyErrorCases: PropTypes.bool
};

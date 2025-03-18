import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import {getConfigsByLabelId, getLabels} from 'app/reducers/service';
import {getService} from 'app/reducers/index';
import {fetchSchema} from 'app/actions/schema';
import {fetchServiceConfigs, changeLabelParent} from 'app/actions/service';
import {fetchServiceSbsProfile} from 'app/actions/service/sbs';
import {openLabelCreateModal} from 'app/actions/create-label';

import Bem from 'app/components/bem/bem';
import Preloader from 'app/components/preloader/preloader';
import InfiniteList from 'app/components/infinite-list/infinite-list';
import Breadcrumbs from 'app/components/breadcrumbs/breadcrumbs';
import Tree from 'app/components/tree/tree';
import ServiceConfigsItem from './__item/service-configs__item';
import Splitter from 'app/components/splitter/splitter';

import Checkbox from 'lego-on-react/src/components/checkbox/checkbox.react';

import i18n from 'app/lib/i18n';

import {serviceType} from 'app/types';

import './service-configs.css';

class ServiceConfigs extends React.Component {
    constructor() {
        super();

        this.state = {
            showArchived: false
        };

        this.onAdd = this.onAdd.bind(this);
        this.onMove = this.onMove.bind(this);
        this.onShowArchivedChange = this.onShowArchivedChange.bind(this);
        this.renderRightComponent = this.renderRightComponent.bind(this);
        this.renderLeftComponent = this.renderLeftComponent.bind(this);
    }

    componentDidMount() {
        this.props.fetchServiceSbsProfile();
        this.props.fetchSchema(this.props.labelId);
    }

    buildPath(tree, labelId) {
        if (!tree || !tree.ROOT) {
            return labelId;
        }

        const queue = [['ROOT', tree.ROOT]];
        while (queue.length) {
            const [name, current] = queue.pop();

            for (let i in current) {
                if (Object.prototype.hasOwnProperty.call(current, i)) {
                    if (i === labelId) {
                        return name + '/' + i;
                    }
                    queue.push([name + '/' + i, current[i]]);
                }
            }
        }

        return labelId;
    }

    breadcrumbs() {
        const path = this.buildPath(this.props.labels, this.props.labelId);
        const parts = path.split('/');

        return parts.map(part => ({
            title: part,
            url: `/service/${this.props.service.id}/label/${part}`
        }));
    }

    onShowArchivedChange() {
        this.setState(state => ({
            showArchived: !state.showArchived
        }));
    }

    onAdd(item) {
        this.props.openCreateLabelModal(this.props.service.id, item.name);
    }

    onMove(who, where) {
        this.props.changeLabelParent(who, where, this.props.service.id);
    }

    renderLeftComponent() {
        return (
            <Tree
                loading={this.props.labelsLoading}
                serviceId={this.props.service.id}
                activeId={this.props.labelId}
                tree={this.props.labels}
                onAdd={this.onAdd}
                onMove={this.onMove} />
        );
    }

    renderRightComponent() {
        return (
            <Bem
                key='configs-list'
                block='service-configs'
                elem='configs-list'>
                <Bem
                    key='filters'
                    block='service-configs'
                    elem='filters'>
                    <Bem
                        key='breadcrumbs'
                        block='service-configs'
                        elem='breadcrumbs'>
                        <Breadcrumbs
                            path={this.breadcrumbs()} />
                    </Bem>
                    <Checkbox
                        theme='normal'
                        size='s'
                        tone='grey'
                        view='default'
                        checked={this.state.showArchived}
                        onChange={this.onShowArchivedChange}>
                        {i18n('service-page', 'archived-configs')}
                    </Checkbox>
                </Bem>
                <Bem
                    key='wrapper'
                    block='service-configs'
                    elem='wrapper'>
                    <InfiniteList
                        key='list'
                        wrapperMix={{
                            block: 'service-configs',
                            elem: 'scroll-wrapper'
                        }}
                        filters={{
                            show_archived: this.state.showArchived,
                            labelId: this.props.labelId
                        }}
                        requestMore={this.props.fetchServiceConfigs}
                        total={this.props.total}
                        items={this.props.configs}
                        Item={ServiceConfigsItem} />
                </Bem>
            </Bem>
        );
    }

    render() {
        return (
            <Bem
                block='service-configs'>
                {!this.props.service ?
                    <Preloader key='preloader' /> :
                    <Splitter
                        componentLocalStorageName='service_configs'
                        leftComponent={this.renderLeftComponent()}
                        rightComponent={this.renderRightComponent()} />
                }
            </Bem>
        );
    }
}

ServiceConfigs.propTypes = {
    service: serviceType.isRequired,
    labelId: PropTypes.string.isRequired,
    labels: PropTypes.object,
    labelsLoading: PropTypes.bool.isRequired,
    configs: PropTypes.array,
    total: PropTypes.number,
    fetchServiceConfigs: PropTypes.func,
    fetchServiceSbsProfile: PropTypes.func.isRequired,
    openCreateLabelModal: PropTypes.func.isRequired,
    changeLabelParent: PropTypes.func.isRequired,
    fetchSchema: PropTypes.func.isRequired
};

export default connect((state, props) => {
    const service = getService(state);
    const configs = getConfigsByLabelId(service, props.labelId);
    const labels = getLabels(service);

    return {
        labels: labels.data,
        labelsLoading: labels.loading,
        configs: configs.items,
        total: configs.total
    };
}, (dispatch, props) => {
    return {
        fetchServiceConfigs: (offset, limit, filters) => {
            return dispatch(fetchServiceConfigs(props.labelId, offset, limit, filters));
        },
        changeLabelParent: (labelId, parentId, serviceId) => {
            return dispatch(changeLabelParent(labelId, parentId, serviceId));
        },
        fetchServiceSbsProfile: serviceId => {
            return dispatch(fetchServiceSbsProfile(serviceId));
        },
        openCreateLabelModal: (serviceId, labelId) => {
            return dispatch(openLabelCreateModal(serviceId, labelId));
        },
        fetchSchema: labelId => {
            return dispatch(fetchSchema(labelId));
        }
    };
})(ServiceConfigs);

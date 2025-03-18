import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import Table from 'app/components/table/table';
import Preloader from 'app/components/preloader/preloader';
import Bem from 'app/components/bem/bem';

import {getService} from 'app/reducers';
import {getSbsLogs} from 'app/reducers/service';
import {fetchSbsLogs} from 'app/actions/service/sbs';
import {sbsMetaLogs} from 'app/types';
import i18n from 'app/lib/i18n';

import './sbs-table-logs.css';

class ServiceTableLogs extends React.Component {
    componentDidMount() {
        this.props.fetchSbsLogs(this.props.resId, this.props.reqId, this.props.type);
    }

    render() {
        return (
            !this.props.logs.loaded ?
                <Preloader /> :
                <Bem
                    block='service-sbs-table-logs'>
                    {!this.props.logs.data.total ?
                        <Bem
                            key='sbs-empty-table'
                            block='service-sbs-table-logs'
                            elem='empty'>
                            {i18n('sbs', 'not-data')}
                        </Bem> :
                        <Table
                            head={this.props.logs.schema}
                            body={this.props.logs.data.items} />
                    }
                </Bem>
        );
    }
}

ServiceTableLogs.propTypes = {
    fetchSbsLogs: PropTypes.func.isRequired,
    resId: PropTypes.string.isRequired,
    reqId: PropTypes.string.isRequired,
    type: PropTypes.string.isRequired,
    logs: sbsMetaLogs
};

export default connect(state => {
    return {
        logs: getSbsLogs(getService(state))
    };
}, dispatch => {
    return {
        fetchSbsLogs: (runId, reqId, type) => {
            dispatch(fetchSbsLogs(runId, reqId, type));
        }
    };
})(ServiceTableLogs);

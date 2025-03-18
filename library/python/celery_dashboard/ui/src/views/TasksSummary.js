import React from 'react';
import MaterialTable from 'material-table';
import tableIcons from '../elements/MaterialTableIcons';
import requests from '../utils/Requests';
import {Link} from 'react-router-dom';
import DiffDate from '../utils/DiffDate';
import { ui_prefix, api_prefix } from '../Constants'


class TasksSummaryView extends React.Component {
    render() {
        return <MaterialTable
            icons={tableIcons}
            title='Task info'
            columns={[
                {
                    title: 'Task name',
                    field: 'name',
                    render: rowData => {
                        let url = `${ui_prefix}/task/${rowData.name}/`
                        return <Link to={url}>{rowData.name}</Link>
                    },
                },
                {
                    title: 'Last success',
                    field: 'last_success_dt',
                    cellStyle: { backgroundColor: 'rgba(0,255,0,0.4)' },
                },
                {
                    title: 'Last retrying',
                    field: 'last_retry_dt',
                    cellStyle: { backgroundColor: 'rgba(255,255,0,0.4)' },
                },
                {
                    title: 'Last failed',
                    field: 'last_failed_dt',
                    cellStyle: { backgroundColor: 'rgba(255,0,0,0.4)' },
                },
            ]}
            options={{
                headerStyle: {
                    backgroundColor: '#01579b',
                    color: '#FFF',
                },
                paging: false,
                search: false,
            }}
            data={query => {
                const url = `${api_prefix}/tasks/?failed_only=1`;
                return requests.get(url, '')
                    .then(result => {
                        result.forEach(function(item, i, arr) {
                            item.last_success_dt = DiffDate(item.last_success_dt * 1000);
                            item.last_retry_dt =  DiffDate(item.last_retry_dt * 1000);
                            item.last_failed_dt =  DiffDate(item.last_failed_dt * 1000);
                        })
                        return ({
                            data: result,
                            page: 0,
                            totalCount: result.length,
                        })
                    })
                }
            }
        />
    }
}

export default TasksSummaryView;

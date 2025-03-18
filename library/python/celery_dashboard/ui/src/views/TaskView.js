import React from 'react';
import MaterialTable from 'material-table';
import tableIcons from '../elements/MaterialTableIcons';
import requests from '../utils/Requests';
import {ArgsKwargsToList} from '../utils/ValueList';
import '../css/MuiListItem-root.css';
import { api_prefix } from '../Constants'
import FailedTaskCard from "../elements/FailedTaskCard";
import DiffDate from "../utils/DiffDate";


class TaskView extends React.Component {
    render() {
        let {task_name} = this.props.match.params;
        return <MaterialTable
            icons={tableIcons}
            title={task_name}
            columns={[
                {
                    title: 'args + kwargs',
                    render: rowData => {
                        return ArgsKwargsToList(rowData.args, rowData.kwargs)
                    }
                },
                {
                    title: 'Retry count',
                    field: 'retry_count',
                    cellStyle: {
                        backgroundColor: 'rgba(255,255,0,0.4)',
                    },
                },
                {
                    title: 'Last retry',
                    field: 'last_retry_dt',
                    cellStyle: {
                        backgroundColor: 'rgba(255,255,0,0.4)',
                    },
                },
                {
                    title: 'Failed count',
                    field: 'failed_count',
                    cellStyle: {
                        backgroundColor: 'rgba(255,0,0,0.4)',
                    },
                },
                {
                    title: 'Last failed',
                    field: 'last_failed_dt',
                    cellStyle: {
                        backgroundColor: 'rgba(255,0,0,0.4)',
                    },
                },
                {
                    title: 'Task ID',
                    field: 'task_id',
                },
                {
                    title: 'Traceback',
                    render: rowData => {
                        return <FailedTaskCard task_info={rowData} caption={rowData.exception}/>
                    },
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
                const url = `${api_prefix}/tasks/${task_name}/`;
                return requests.get(url, '')
                    .then(result => {
                        result.forEach(function(item, i, arr) {
                            item.last_failed_dt = DiffDate(item.last_failed_dt * 1000);
                            item.last_retry_dt = DiffDate(item.last_retry_dt * 1000);
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

export default TaskView;

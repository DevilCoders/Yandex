import React, { Component } from 'react';
import FailedTaskCard from './FailedTaskCard';
import MoreTasksInfoCard from './MoreTasksInfoCard';
import '../css/FailedTasksContainer.css'


const max_failed_count = 10


class FailedTasksContainer extends Component {
    render() {
        let data_len = this.props.data.length
        let data = this.props.data


        data = data.map(row => (
            <FailedTaskCard task_info={row}/>
        ))

        let first_data = data.slice(0, max_failed_count)
        let second_data = data.slice(max_failed_count, data_len)

        if (data_len > max_failed_count)
            first_data.push(
                <MoreTasksInfoCard
                    count={data_len - max_failed_count}
                    data={second_data}
                />
            )

        return <section className='FailedTasksContainer'> { first_data } </section>
    }
}

export default FailedTasksContainer;

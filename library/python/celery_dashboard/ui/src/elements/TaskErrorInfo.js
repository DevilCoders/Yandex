import React from 'react';
import requests from '../utils/Requests';
import FailedTasksContainer from './FailedTasksContainer';


class TaskErrorInfo extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            error: null,
            isLoaded: false,
            items: []
        };
        this.url = props.url;
    }

    componentDidMount() {
        requests.get(this.url, '').then(
            (result) => {
                this.setState({
                    isLoaded: true,
                    items: result
                })
            },
            (error) => {
                this.setState({
                    isLoaded: true,
                    error
                })
            }
        )
    }

    render() {
        const {error, isLoaded, items} = this.state;

        if (error) {
            return <div>Error: {error.message}</div>;
        } else if (!isLoaded) {
            return <div>Loading...</div>;
        } else {
            return <FailedTasksContainer
                data={ items }
            />
        }
    }
}

export default TaskErrorInfo;

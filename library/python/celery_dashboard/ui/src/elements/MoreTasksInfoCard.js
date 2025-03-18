import React from 'react';
import '../css/MoreTasksInfoCard.css'


class MoreTasksInfoCard extends React.Component {
    constructor(props) {
        super(props)
        this.state = {
            is_expand: false,
            data: props.data,
            count: props.count
        }
    }

    render() {
        return (
            this.state.is_expand ? (
                this.state.data
            ) : (
                <div className='MoreTasksInfoCard'>
                    <button
                        className="button"
                        onClick={e => this.setState({is_expand: true})}
                    >
                        { this.state.count } More
                    </button>
                </div>
            )
        )
    }
}

export default MoreTasksInfoCard;

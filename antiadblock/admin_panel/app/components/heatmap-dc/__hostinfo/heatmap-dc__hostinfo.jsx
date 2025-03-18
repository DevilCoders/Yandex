import React from 'react';
import Tooltip from 'lego-on-react/src/components/tooltip/tooltip.react';
import {heatmapItemType} from '../../../types';
import './heatmap-dc__hostinfo.css';

export default class HeatmapDCHostInfo extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            tooltipVisible: false
        };
        this.onMouseOver = this.onMouseOver.bind(this);
        this.onMouseLeave = this.onMouseLeave.bind(this);
    }

    onMouseOver() {
        this.setState({tooltipVisible: true});
    }

    onMouseLeave() {
        this.setState({tooltipVisible: false});
    }

    render() {
        return (
            <div className='heatmap-dc__hostinfo' onMouseOver={this.onMouseOver} onMouseLeave={this.onMouseLeave}>
                <Tooltip
                    key={'hostinfo_tooltip' + this.props.hostData.host}
                    hasTail
                    visible={this.state.tooltipVisible}
                    anchor={this}
                    theme='normal'
                    view='classic'
                    size='xs'
                    autoclosable>
                    <span>
                        {this.props.hostData.host}
                        <br />
                        {
                            (this.props.hostData.cpu_load) ?
                                <span><p />cpu load: {this.props.hostData.cpu_load.toFixed(1)}%</span> :
                                <span />
                        }
                        <br />
                        {
                            (this.props.hostData.rps) ?
                                <span><p />rps: {this.props.hostData.rps}</span> :
                                <span />
                        }
                    </span>
                </Tooltip>
            </div>
        );
    }
}

HeatmapDCHostInfo.propTypes = {
    hostData: heatmapItemType
};

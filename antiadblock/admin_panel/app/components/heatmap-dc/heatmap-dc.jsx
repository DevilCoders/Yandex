import React from 'react';
import Bem from 'app/components/bem/bem';
import HeatmapDCHostInfo from './__hostinfo/heatmap-dc__hostinfo';
import PropTypes from 'prop-types';
import {heatmapItemType} from '../../types';
import './heatmap-dc.css';

export default class HeatmapDC extends React.Component {
    static getColor(hostData) {
        if (!hostData || hostData.cpu_load === undefined) {
            return 'rgb(0,0,0)';
        }
        let val = hostData.cpu_load / 100,
            hue = 225 - (225 * val);

        return `hsl(${hue}, 100%, 50%)`;
    }

    render() {
        const colsCount = 20;
        let rows = [];

        this.props.hosts.forEach((val, idx) => {
            let i = Math.floor(idx / colsCount);

            val.key = this.props.dc + '_' + idx;
            if (rows[i]) {
                rows[i].push(val);
            } else {
                rows[i] = [val];
            }
        });
        return (
            <Bem block='heatmap-dc'>
                <table className='heatmap-dc__dc-table'>
                    <tbody>
                        {rows.map(row => (
                            <tr key={row.key} id={row.key} className='heatmap-dc__row'>
                                {row.map(hostData => (
                                    <td key={hostData.host}
                                        className='heatmap-dc__inst'
                                        style={{backgroundColor: HeatmapDC.getColor(hostData)}}>
                                        <HeatmapDCHostInfo key={hostData.host + '_tooltip'} hostData={hostData} />
                                    </td>
                                ))}
                            </tr>
                            )
                        )}
                    </tbody>
                </table>
            </Bem>
        );
    }
}

HeatmapDC.propTypes = {
    hosts: PropTypes.arrayOf(heatmapItemType).isRequired,
    dc: PropTypes.string.isRequired
};

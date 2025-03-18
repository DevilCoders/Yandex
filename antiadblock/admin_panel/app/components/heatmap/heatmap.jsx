import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import {getHeatmap} from 'app/reducers/index';

import Bem from 'app/components/bem/bem';
import Preloader from 'app/components/preloader/preloader';

import './heatmap.css';

import {heatmapItemType} from 'app/types';

import {fetchHeatmap} from 'app/actions/heatmap';
import HeatmapDC from 'app/components/heatmap-dc/heatmap-dc';
import i18n from 'app/lib/i18n';

const AWAIT_TIME = 1000;

class Heatmap extends React.Component {
    constructor(props) {
        super(props);

        this._timer = null;

        this._isFetching = false;
    }

    _fetchHeatmap() {
        if (this._isFetching) {
            return;
        }

        this._isFetching = true;

        this.props.fetchHeatmap()
            .then(() => {
                this._isFetching = false;
            });
    }

    componentDidMount() {
        this._timer = setInterval(() => this._fetchHeatmap(), AWAIT_TIME);
    }

    componentWillUnmount() {
        clearInterval(this._timer);
    }

    render() {
        let dcDict = {},
         heatmap = this.props.heatmapData.result.heatmap,
         ts = this.props.heatmapData.result.ts;

        if (this.props.dataLoaded) {
            dcDict = heatmap.reduce(function (map, obj) {
                if (!map[obj.dc]) {
                    map[obj.dc] = [];
                }
                map[obj.dc].push(obj);
                return map;
            }, {});
        }

        return (
            <Bem block='heatmap'>
                {!this.props.dataLoaded ?
                    <Preloader /> :
                    [
                        <Bem
                            key='header'
                            block='heatmap'
                            elem='header'>
                            <Bem
                                block='heatmap'
                                elem='title'>
                                {i18n('dashboard', 'heatmap-title')}
                            </Bem>
                            <Bem
                                block='heatmap'
                                elem='timestamp'>
                                <span>{new Date(ts * 1000).toLocaleString()}</span>
                            </Bem>
                        </Bem>,
                        (
                            <div key='heatmap-content'>
                                <div className='heatmap__groups-container'>
                                    {Object.entries(dcDict).map(dcEntry => (
                                        <table key={dcEntry[0]}>
                                            <tbody>
                                                <tr>
                                                    <td className='heatmap__dc-name'>
                                                        {dcEntry[0]}
                                                    </td>
                                                    <td>
                                                        <HeatmapDC hosts={dcEntry[1]} dc={dcEntry[0]} />
                                                    </td>
                                                </tr>
                                            </tbody>
                                        </table>
                                    ))}
                                </div>
                            </div>
                        )
                    ]
                }
            </Bem>
        );
    }
}

Heatmap.propTypes = {
    heatmapData: PropTypes.shape({
        result: PropTypes.shape({
            ts: PropTypes.number.isRequired,
            heatmap: PropTypes.arrayOf(heatmapItemType).isRequired
        }).isRequired
    }).isRequired,
    dataLoaded: PropTypes.bool.isRequired,
    fetchHeatmap: PropTypes.func.isRequired
};

export default connect(state => {
    const heatmapState = getHeatmap(state);
    return {
        heatmapData: heatmapState.heatmapData,
        dataLoaded: heatmapState.dataLoaded
    };
}, dispatch => {
    return {
        fetchHeatmap: () => {
            return dispatch(fetchHeatmap());
        }
    };
})(Heatmap);

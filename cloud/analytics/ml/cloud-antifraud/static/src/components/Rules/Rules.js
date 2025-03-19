
// import withSize from 'react-sizeme';
import HeatmapRules from './HeatmapRules';
import BarRules from './BarRules';

import React from 'react';



function Rules() {
    
    return ( 
    <div className="container-fluid" style={{ backgroundColor: "rgb(249, 249, 249)" }}>
        <div class="row">

        </div>
        <div className="row">
            <HeatmapRules rule='created_rule'/>
        </div>

        <div className="row">
            <BarRules rule='created_rule'/>
        </div>

        <div className="row">
            <HeatmapRules rule='cloud_rule'/>
        </div>

       

        <div className="row">
            <BarRules rule='cloud_rule'/>
        </div>
    </div>);
}

export default Rules;

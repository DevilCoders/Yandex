import YandexSlider from '../Slider/Slider';
import Typography from '@material-ui/core/Typography';
// import { Dropdown } from 'react-materialize';
import Plot from 'react-plotly.js';
import React, { useState, useEffect } from 'react';
import update from 'immutability-helper';
import withSize from 'react-sizeme';

import axios from 'axios';

function  valuetext(value) {
    return new Date(value * 1000).toLocaleDateString("ru-RU")
  };

function BarRules({ rule, size }) {
    const [data, setData] = useState({ time_range: {}, 
                                       current_range:[], 
                                       bar: {
                                        rules: [],
                                        detected: [],
                                        false_positives: [],
                                    }
                                });

    
    useEffect((data) => {
        async function fetchData() {
            const result = await axios.get('/api/time_range');
            const bar = await axios.get(`/api/rules/bar?rules=${rule}`);
            setData((data) => update(data, 
                {
                    time_range: { $set: result.data },
                    current_range: { $set: [result.data.min_time, result.data.max_time] },
                    bar: {
                                rules:{$set:bar.data.rules},
                                detected:{$set:bar.data.detected},
                                false_positives:{$set:bar.data.false_positives}
                         }
                }   
                
                ));
        }
        fetchData();
      }, [rule]);
    
    const  handleSliderChange =  rules => async (event, newValue) => {
        const bar = await axios.get(`/api/rules/bar?from=${newValue[0]}&to=${newValue[1]}&rules=${rules}`);
        setData(update(data, {
                current_range: { $set: newValue } ,
                bar: {
                    rules:{$set:bar.data.rules},
                    detected:{$set:bar.data.detected},
                    false_positives:{$set:bar.data.false_positives}
        }}));

    };

  
    // console.log(data)
    return (
        
        <div className="container" style={{ width: "100%" }}>
            <div className="card z-depth-0">
            <div className="card-content black-text">


            <div className="MTableToolbar-title-9"><h6 className="MuiTypography-root MuiTypography-h6"
              style={{ whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis" }}>
                  {rule==='created_rule' ? 'Created Rule' : 'Cloud Rule'}s: Detected vs False Positives
               </h6>

            </div>

            <Typography id="range-slider" gutterBottom>
                Billing account creation range: {valuetext(data.current_range[0])} ... {valuetext(data.current_range[1])}
            </Typography>

            <YandexSlider
                value={data.current_range}
                onChange={(event, newValue) => setData(update(data,  { current_range: { $set: newValue } }))}
                onChangeCommitted={handleSliderChange(rule)}
                valueLabelDisplay="off"
                step={86400}
                aria-labelledby="discrete-range-slider"
                min={data.time_range.min_time}
                max={data.time_range.max_time}

                valueLabelFormat={valuetext}
                />
        <Plot data={[
                        {
                            x: data.bar.rules,
                            y: data.bar.detected,
                            name: 'Detected',
                            type: 'bar',
                            orientation: 'v'
                          },
                          {
                            x: data.bar.rules,
                            y: data.bar.false_positives,
                            name: 'False Positives',
                            type: 'bar',
                            orientation: 'v'
                          }
                        ]}
              layout={{ //title: '',
                        barmode: 'stack',
                        hovermode: true,
                        width: size.width * 0.97,
                        margin: {
                        l: 50, r: 50, b: 50, t: 50, pad: 4
                        },
                        autosize: false 
                    }}
              config={{ responsive: true, displayModeBar: true, displaylogo: false }}
            //   config={this.state.config}
            //   style={this.state.style}
            //   onSelected={this.onSelected}
              id='rules_heatmap'

            />

            
            </div>

            </div>
            </div>

    );
    }


export default withSize()(BarRules);

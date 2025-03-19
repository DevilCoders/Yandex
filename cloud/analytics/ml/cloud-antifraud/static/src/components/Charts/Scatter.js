import React, { Component } from 'react';
import Plot from 'react-plotly.js';
import update from 'immutability-helper';
import sizeMe from 'react-sizeme';
import Loader from '../Loader/Loader'
import YandexSlider from '../Slider/Slider';
import Typography from '@material-ui/core/Typography';
import { Dropdown } from 'react-materialize';


class Scatter extends Component {
  constructor(props) {
    super(props);
    this.resetLabels = this.resetLabels.bind(this)
    let resetColorsIcon = {
      'width': 342,
      'height': 392,
      'path': 'M299 235q0 14-3 28L112 79q14-19 28.5-37.5T163 14l8-10q5 6 13.5 16.5t30.5 40t39 56.5t31 60.5t14 57.5zm-19 66l58 59l-27 27l-56-56q-36 32-84 32q-53 0-90.5-37.5T43 235q0-35 28-88L0 76l27-28l154 155z',
      // 'transform': 'rotate(360deg)'
    }

    let modeBarButtons =
      [{
        name: 'Reset Labels',
        icon: resetColorsIcon,
        click: this.resetLabels
      }];


    this.state = {
      time_range: {},
      full_data: {},
      revision: 0,
      data: [],
      layout: {},
      error: false,
      config: { responsive: true, modeBarButtonsToAdd: modeBarButtons, displayModeBar: true, displaylogo: false },
      loading: true
    }


    this.onSelected = this.onSelected.bind(this)
    this.updateColors = this.updateColors.bind(this)
    this.handleSliderChangeCommited = this.handleSliderChangeCommited.bind(this)
    this.highlightActive = this.highlightActive.bind(this)
    this.highlightRulesList = this.highlightRulesList.bind(this)
    this.highlightRule = this.highlightRule.bind(this)



  }

  

  static shortColorScale = [[0, 'rgba(138, 138, 138,0.3)'], [1 / 2, 'rgb(121, 209, 67,0.3)'], [1, 'rgb(252, 32, 80,0.3)']]
  static longColorScale = [[0, 'rgba(138, 138, 138,0.3)'], [1 / 6, 'rgb(121, 209, 67,0.3)'], [2 / 6, 'rgb(252, 32, 80, 0.3)'], [1, "rgb(63, 142, 238)"]]

  static loadPlotData(props) {
    var colorscale = []
    if (props.full_data['y'].includes(5)) {
      colorscale = [...Scatter.longColorScale]
    } else {
      colorscale = [...Scatter.shortColorScale]
    }

    var text = []
    for (var i = 0; i < props.full_data['y'].length; i++) {
        text.push(`${props.full_data['account_name'][i]
                  } <br>Billing Account ID: ${props.full_data['billing_account_id'][i]
                  } <br>Cloud ID: ${props.full_data['cloud_id'][i]} `)
    }

    var data = [{
      x: props.full_data['u0'],
      y: props.full_data['u1'],
      text:text,
      type: 'scattergl',
      mode: 'markers',
      transforms: [],
      marker: {
        color: [...props.full_data['y']],
        colorscale: colorscale,
      },
    }]

    var layout = {
      hovermode: false,
      width: props.size.width * 0.97,
      margin: {
        l: 50, r: 50, b: 50, t: 50, pad: 4
      },
      autosize: false
    }

    var min_time = Math.min(...props.full_data['ba_time']);
    var max_time = Math.max(...props.full_data['ba_time']);

    var time_range = {
      min_time: min_time,
      max_time: max_time,
      current_range: [min_time, max_time]
    }


    return { data: data, layout: layout, time_range: time_range }
  }

  static getDerivedStateFromProps(props, state) {
    if ((props.loading === false) & (state.loading === true)) {
      let new_data = Scatter.loadPlotData(props);
      let curr_state = update(state,
        {
          data: { $set: new_data.data },
          full_data: { $set: props.full_data },

          layout: { $set: new_data.layout },
          time_range: { $set: new_data.time_range },
          revision: { $set: state.revision + 1 },
          loading: { $set: false },
        })
      return curr_state
    }
    if (props.loading === false) {
      let curr_state = update(state,
        {
          highlightPointsIds: { $set: props.highlightPointsIds },
          layout: { width: { $set: props.size.width * 0.97 } }
        })
      

        if (props.highlightPointsIds !== state.highlightPointsIds) {
          var colors = [...state.data[0].marker.color]
          var points_annotations = []

          const colorscale = (props.highlightPointsIds.length > 0) ?  [...Scatter.longColorScale] : [...Scatter.shortColorScale]
          props.highlightPointsIds.forEach(point => {
              colors[point.pointIndex]=5
              points_annotations.push({
                x: state.full_data['u0'][point.pointIndex],
                y: state.full_data['u1'][point.pointIndex],
                xref: 'x',
                yref: 'y',
                text: state.full_data['billing_account_id'][point.pointIndex],
                showarrow: true,
                arrowhead: 7,
                ax: 0,
                ay: -40
              })
            
          });
          
          let index = 0;
          curr_state = update(curr_state,
            {
              data: {
                [index]: {
                  marker: {
                    color: { $set: colors },
                    colorscale: { $set: colorscale }
                  }
                }
              },
              layout:{annotations: {$set: points_annotations}}
            }
          
          );          
        }
      return curr_state
    }
    
    return null;
  }

  updateColors = colors => {
    let index = 0;
    var new_state = update(this.state,
      {
        data: {
          [index]: {
            marker: {
              color: { $set: colors },
              colorscale: { $set: [...Scatter.longColorScale] }
            }
          }
        },
        revision: { $set: this.state.revision + 1 }
      }
    
    );
    return new_state
  }

  onSelected = selected_data => {
    this.props.onSelected(selected_data.points);
    var curr_state = update(this.state,
      {
        revision: { $set: this.state.revision + 1 }
      })
    curr_state.data[0].marker.color = [...curr_state.data[0].marker.color]

    selected_data.points.forEach(point => {
      curr_state.data[0].marker.color[point.pointIndex] = 5

    });

    var new_state = this.updateColors(curr_state.data[0].marker.color)
    this.setState(new_state);

  }

  highlightActive = event => {
    event.preventDefault();
    var colors = [...this.state.data[0].marker.color]
    var statuses = this.state.full_data['ba_state']
    for (var i = 0; i < colors.length; i++) {
        if(statuses[i] === 'active'){
          colors[i] = 5
        }
    }

    var new_state = this.updateColors(colors)
    this.setState(new_state);

  }

  

  resetLabels() {
    let index = 0;
    var new_state = update(this.state,
      {
        data: {
          [index]: {
            marker: {
              color: { $set: [...this.state.full_data['y_back']] },
              colorscale: { $set: [...Scatter.shortColorScale] }
            }
          }
        },
        layout:{annotations: {$set: []}},
        revision: { $set: this.state.revision + 1 }
      }
    );
    this.setState(new_state);

  }

  handleSliderChange = (event, newValue) => {
    var new_state = update(this.state, { time_range: { current_range: { $set: newValue } } })
    this.setState(new_state);
  }

  handleSliderChangeCommited = (event, newValue) => {
    let index = 0;
    var change_data = update(this.state, {
      data: {
        [index]: {
          transforms: {
            $set: [{
              type: 'filter',
              target: this.state.full_data['ba_time'],
              operation: '>=',
              value: newValue[0]
            },
            {
              type: 'filter',
              target: this.state.full_data['ba_time'],
              operation: '<=',
              value: newValue[1]
            },
            ]
          }
        },
        revision: { $set: this.state.revision + 1 }
      }
    });
    this.setState(change_data);
  };

  valuetext(value) {
    return new Date(value * 1000).toLocaleDateString("ru-RU");
  }

  highlightRulesList(){
    const created_rules = Array.from({length: 7}, (x, i) => ['created_rules', i.toString()])
    const cloud_rules = Array.from({length: 11}, (x, i) => ['cloud_rules', i.toString()])
    const rules = created_rules.concat(cloud_rules)
    return rules.map((rule) =>{
      return <a href="/" onClick={this.highlightRule(rule[1])(rule[0])}> {rule[0]}{rule[1]} </a>
    } )
  }

  highlightRule = rule_n =>rule_column=>event =>{
    event.preventDefault();
    console.log(rule_column, rule_n)
    var colors = [...this.state.data[0].marker.color]
    var rules = this.state.full_data[rule_column]
    for (var i = 0; i < colors.length; i++) {
        if(rules[i].toString().split(',').includes(rule_n)){
          colors[i] = 5
        }
    }
    var new_state = this.updateColors(colors)
    this.setState(new_state);
  }

  render() {
    if (this.state.loading) return <Loader />;



    return (
      <div className="container" style={{ width: "100%" }}>
        <div className="card z-depth-0">
          <div className="card-content black-text">


            <div className="MTableToolbar-title-9"><h6 className="MuiTypography-root MuiTypography-h6"
              style={{ whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis" }}>Cloud Accounts
               <Dropdown
                id="Dropdown_6"
                options={{
                  alignment: 'right', coverTrigger: false, constrainWidth:false

                }}
                trigger={<a href="/" style={{ float: 'right' }}><i className="material-icons right" >more_vert</i></a>} >
                <a href="/" onClick={this.highlightActive}> Highlight Active </a>
                {this.highlightRulesList()}

              </Dropdown></h6>

            </div>

            <Typography id="range-slider" gutterBottom>
              Billing account creation range: {this.valuetext(this.state.time_range.current_range[0])} ... {this.valuetext(this.state.time_range.current_range[1])}
            </Typography>
            <YandexSlider
              value={this.state.time_range.current_range}
              onChangeCommitted={this.handleSliderChangeCommited}
              onChange={this.handleSliderChange}
              valueLabelDisplay="off"
              step={86400}
              aria-labelledby="discrete-range-slider"
              min={this.state.time_range.min_time}
              max={this.state.time_range.max_time}

            // valueLabelFormat={this.valuetext}
            />



            <Plot data={this.state.data}
              layout={this.state.layout}
              config={this.state.config}
              style={this.state.style}
              onSelected={this.onSelected}
              id='myDiv'

            />
          </div>


        </div>
      </div>
    );
  }
}


export default sizeMe()(Scatter);

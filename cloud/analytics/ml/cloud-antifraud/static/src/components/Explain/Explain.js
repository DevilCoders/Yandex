

import update from 'immutability-helper';
import MaterialTable from 'material-table';
import { Paper } from '@material-ui/core';
import Plot from 'react-plotly.js';
import { SizeMe } from 'react-sizeme'

import React from 'react';
import queryString from 'query-string';

class Explain extends React.Component {
    constructor(props) {
        super(props);
        this.state = { sample: {}, 
                       explanation:{}, 
                       account_name: '', 
                       billing_account_id:'', 
                       cloud_id:'' ,
                       loading:true,
                       confidence:0
                    }; 
    }

    componentDidMount() {
        const { location: { search } } = this.props;
        const values = queryString.parse(search);
        // Use the values to whatever you want.

        fetch(`/api/explain?billing_account_id=${values.billing_account_id}&cloud_id=${values.cloud_id}&account_name=${values.account_name}`, { method: "GET" })
        .then((response) => {
          return response.json();
        })
        .then((data) => {
          var curr_state = update(this.state,
            {
              account_name: { $set: values.account_name },
              billing_account_id: { $set: values.billing_account_id },
              cloud_id: { $set: values.cloud_id },
              sample: { $set: data.sample },
              explanation: { $set: data.explanation },
              loading: { $set: false },
              confidence: { $set: parseFloat(data.confidence) },
            })
          this.setState(curr_state)
        })
        .catch((error) => {
          this.setState({
            error: true
          })
        });

        console.log(values)
    }

    render() {
        if(this.state.loading === true){ 
            return <h4> Loading...</h4>
        }
        const column_names = Object.keys(this.state.sample)
        var columns = [];
        column_names.forEach(col =>
            { columns.push({title:col, field:col})
        });
        console.log(columns)
        var rows = [];
        for(let i=0; i < this.state.sample[column_names[0]].length; i++){
            const prop_name = this.state.sample['Property Name'][i]
            var prop_value = this.state.sample['Property Value'][i]
            var shap_value = this.state.sample['SHAP'][i]
            if (['ba_time', 'time'].includes(prop_name)){
                prop_value  = (new Date(prop_value * 1000)).toLocaleDateString("ru-RU") 
            }
            let row = {'Property Name':prop_name, 'Property Value':prop_value, 'SHAP':shap_value };
            rows.push(row);
        }

  

    return <div>
                <h4> {this.state.account_name}</h4>
                <p>Confidence that this is fraud {(this.state.confidence*100).toFixed(2)}%</p>

                <SizeMe>{({ size }) => <Plot
                    data={[
                            {type: 'bar', 
                            x: this.state.explanation.x, 
                            y: this.state.explanation.y,
                            marker:{color:this.state.explanation.y.map( (y) => y < 0 ? 'blue' : 'orange')}
                        },

                    ]}
                    layout={ {width: size.width, height: size.height*0.15, title: 'LIME features weights'} }
                />}</SizeMe>
                <MaterialTable
                    columns={columns}
                    data={rows}
                    options={{
                        exportButton: true,
                        emptyRowsWhenPaging:true

                    
                    }}
                    title='Cloud properties'
                    localization={{
                        body:{emptyDataSourceMessage:'No data'}}}
                    components={{
                        Container: props => <Paper {...props} elevation={0}/>
                    }} />
            </div>
    }
}

export default Explain;

import React, { Component } from 'react';
import MaterialTable from 'material-table';
import { Paper, Tooltip } from '@material-ui/core';
import update from 'immutability-helper';
import { Link } from 'react-router-dom';

class Table extends Component {
  constructor(props) {
    super(props);
    this.state= {loading:true, full_data:{}, selected_data_points:[]}
  }

  static getDerivedStateFromProps(props, state) {
    if (props.loading === false) {
      var curr_state = update(state,
        {loading: { $set: false },
         full_data: { $set: props.full_data},
         selected_data_points: { $set: props.selected_data_points}
        })
      return curr_state
    }
    return null;
  }
  
  render() {
    if (this.state.loading) return   <div/>

    var table_columns =  ['account_name', 
        'ba_state',
        'is_target',
        'is_bad',
        'is_bad_current',
        'billing_account_id',
        'cloud_id',
        'passport_id',
        'trial_puid_diff',
        'ba_puid_diff',
        'vm_count',
        'max_cores',
        'trial_consumption',
        'real_consumption',
        "total_balance",
        "sales_name",
        "n_sim_accounts", 
        "same_ip",
        "same_phone",
        "registration_country",
        'created_rules',   
        'cloud_rules',
        'suspicious_rules',  
        'phone',
        'u0',
        'u1',
        'y_back',
        'is_verified',
        'conf1'
    ];
    

    var rows = [];
    this.state.selected_data_points.forEach(point => {
      var row = {};
      table_columns.forEach(col => {
        const col_value =  this.state.full_data[col][point.pointIndex] 
        if (['ba_time', 'time'].includes(col)){
          row[col]  = new Date(col_value * 1000).toLocaleDateString("ru-RU") 
        } else{
          row[col]  = col_value
        }
        });
      rows.push(row);
    });
    console.log(rows)
    var columns = [];
    if (rows.length>0){
      columns.push({
        field: 'action',
        title: 'Action',
        render: rowData => <div>
                              <Link to={`/explanation?billing_account_id=${rowData.billing_account_id}&cloud_id=${rowData.cloud_id}&account_name=${rowData.account_name}`}  target="_blank"> 
                                  <Tooltip title="Explain">
                                    <i className="material-icons" 
                                      style={{ color: "black", width: 50, borderRadius: '50%'}}>
                                          assignment
                                    </i> 
                                  </Tooltip>
                              </Link>
                              <Link to={`/?billing_account_id=${rowData.billing_account_id}`}  target="_blank"> 
                                  <Tooltip title="Locate">
                                    <i className="material-icons" 
                                      style={{ color: "black", width: 50, borderRadius: '50%'}}>
                                          add_location
                                    </i>
                                  </Tooltip> 
                              </Link>
                            </div>
      })
      table_columns.forEach(col =>
        { 
          var col_obj = null
          if (col==='billing_account_id'){
              col_obj = {title:col, field:col, render: rowData => <a href={`https://backoffice.cloud.yandex.ru/billing/accounts/${rowData.billing_account_id}`}
              rel="noreferrer noopener"
              target="_blank">{rowData.billing_account_id}</a>
            }
          } else if (col==='cloud_id'){
            col_obj = {title:col, field:col, render: rowData => <a href={`https://backoffice.cloud.yandex.ru/clouds/${rowData.cloud_id}`}
            rel="noreferrer noopener"
            target="_blank">{rowData.cloud_id}</a>
           }
          } else {
            col_obj = {title:col, field:col}
          }
          columns.push(col_obj)
      });
    };
    return (
      <div className="container"  style={{width: "100%"}}>
          <div className="card z-depth-0">
            
          <MaterialTable
            columns={columns}
            data={rows}
            title="Selected clouds"
            options={{
              exportButton: true,
              
            }}
            localization={{
              body:{emptyDataSourceMessage:'Select points on scatter plot above with lasso or box selection tool'}}}
            components={{
              Container: props => <Paper {...props} elevation={0}/>
              }}
        />
        </div>
      </div>
    );
  }
}


export default Table;

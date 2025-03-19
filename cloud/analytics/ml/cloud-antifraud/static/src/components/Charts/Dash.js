import React, { Component } from 'react';
import Scatter from './Scatter';
import Table from './Table';
import logo from '../../logo.svg';
import M from 'materialize-css/dist/js/materialize.min.js';
import update from 'immutability-helper';
import queryString from 'query-string';
import { Link } from 'react-router-dom';

import { Autocomplete, Icon } from 'react-materialize';
class Dash extends Component {
  constructor(props) {
    super(props);
    this.state = { selected_data_points: [], full_data: {}, loading: true, highlightPointsIds: [] };
    this.onSelected = this.onSelected.bind(this);
    this.searchName = this.searchName.bind(this);

  }

  onSelected = data => {
    var curr_state = update(this.state,
      {
        selected_data_points: { $set: data }
      })
    this.setState(curr_state)
  }

  componentDidMount() {
    const { location: { search } } = this.props;
    const values = queryString.parse(search);
    var billing_account_id = null
    if ('billing_account_id' in values){
      billing_account_id = values.billing_account_id
    }
    console.log(billing_account_id)

    // values.billing_account_id
    let sidenav = document.querySelector('#slide-out');
    M.Sidenav.init(sidenav, {});
    const url = '/api/features_dash';


    

    fetch(url, { method: "POST" })
      .then((response) => {
        return response.json();
      })
      .then((data) => {
        var autocomplete_dict = {};
        data['account_name'].forEach(element => {
          autocomplete_dict[element] = null
        });

        data['billing_account_id'].forEach(element => {
          autocomplete_dict[element] = null
        });
        var curr_state = update(this.state,
          {
            full_data: { $set: data },
            loading: { $set: false },
            autocomplete: { $set: autocomplete_dict }
          })
        this.setState(curr_state)
        if (billing_account_id!== null){
          this.searchName(billing_account_id)
        }
      })
      .catch((error) => {
        this.setState({
          error: true
        })
      });
  }

  searchName = value => {
    const url = '/api/search';
    const data = {
      account_name: value,
    }
    
    fetch(url, {
      method: 'POST',
      body: JSON.stringify(data),
      headers: {
        'Content-Type': 'application/json'
      }
    }).then(response => response.json())
      .then((data) => {
        var points = []
        data.inds.forEach(el => {points.push({pointIndex:el})});
        var curr_state = update(this.state,
          {
            highlightPointsIds: { $set: [...points] },
            selected_data_points: { $set: [...points] },
          })
        this.setState(curr_state)
      });

  }

  render() {
    return (
      <div className="containwer-fluid" style={{ backgroundColor: "rgb(249, 249, 249)" }}>
        <ul id="slide-out" className="sidenav">
          <li><a href="https://datalens.yandex-team.ru/3oq54os4ytlqz-antifraud-blocking-monitor?tab=18"
            rel="noreferrer noopener"
            target="_blank">Antifraud dashboard</a></li>
          <li><a href="https://wiki.yandex-team.ru/users/bbuvaev/YC-Antifraud/Opisanie-pravil-Antifroda/"
            rel="noreferrer noopener"
            target="_blank">Wiki rules description</a></li>

          <li><a href="https://nirvana.yandex-team.ru/flow/677fa3a8-7185-4428-9c0f-e256189647c7/59f91731-f9ad-4aaa-abad-1e718b364056/graph"
            rel="noreferrer noopener"
            target="_blank">Nirvana offline antifraud workflow</a></li>

          <li><a href="https://yql.yandex-team.ru/Operations/Xs5P2yAsJTdjK_xPn26Sv-3qpBT35SHCaPksnptU80M=?editor_page=main"
                      rel="noreferrer noopener"
                      target="_blank">Offline cloud and suspicious rules</a></li>
          
          <li><a href="https://a.yandex-team.ru/arc/trunk/arcadia/quality/antifraud/scripts/prod/yql_scripts/passport_features.yql"
                      rel="noreferrer noopener"
                      target="_blank">Offline created rules YQL</a></li>
          <li><Link to='/rules' target="_blank"> Rules stats </Link></li>


        </ul>
        <div className="navbar-fixed z-depth-0">
          <nav className="white z-depth-0" style={{ borderBottom: "2px solid #eee" }}>

            <a href='/' data-target="slide-out" className="sidenav-trigger show-on-large" >
              <i className="material-icons" style={{ color: "black" }}>menu</i>
            </a>


            <div className="nav-wrapper  white">
              <a href="/" className="brand-logo">
                <a href="/" className="logo">
                  <img src={logo} alt="Yandex logo" style={{ height: "35px", paddingBottom: "5px" }}></img>
                </a>
              </a>

              <ul className="hide-on-med-and-down right">
                <li>
                  <Autocomplete
                    icon={<Icon>search</Icon>}
                    id="Autocomplete-1"
                    style={{ width: 400 }}
                    options={{
                      data: this.state.autocomplete,
                      minLength: 3,
                      onAutocomplete: this.searchName
                    }}
                    placeholder="Search" />
                </li>
                <li><a href='/'>&nbsp;</a></li>
              </ul>
            </div>
          </nav>
        </div>
        <div className="row">
          <Scatter onSelected={this.onSelected}
            full_data={this.state.full_data}
            loading={this.state.loading}
            highlightPointsIds={this.state.highlightPointsIds} />
        </div>
        <div className="row">
          <Table selected_data_points={this.state.selected_data_points}
            full_data={this.state.full_data}
            loading={this.state.loading} />
        </div>
      </div>
    );
  }
}

export default Dash;

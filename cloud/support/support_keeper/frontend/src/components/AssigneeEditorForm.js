import React from "react";
// import axios from "axios";

class AssigneeEditorForm extends React.Component  {
  constructor(props) {
      super(props);
      this.state = {
          q_id: this.props.queue,
          queue_name: this.props.name,
          is_filter: this.props.is_filter,
          query_text: this.props.query_text,
          ts_warning_limit: this.props.ts_warning_limit,
          ts_danger_limit: this.props.ts_danger_limit,
          crew_warning_limit: this.props.crew_warning_limit,
          crew_danger_limit: this.props.crew_danger_limit,
          assignees: this.props.assignees,
          s_units: this.props.s_units
      }

      this.handleSubmit = this.handleSubmit.bind(this)
      this.handleChange = this.handleChange.bind(this)
      this.handleMultChange =this.handleMultChange.bind(this)
  }
    handleMultChange(selectedItems) {
      let assignees = []
        for (let i=0; i<selectedItems.length; i++)
        {
            assignees.push(selectedItems[i].value)
        }
        console.log(assignees)
      this.setState(
          {'assignees': assignees}
      )
    }
    handleSubmit(event) {
      if (this.props.is_auth) {
          this.props.callback(this.state)


      }
      else
      {
          alert('Login session expired. Relogin, retry')
      }
      event.preventDefault()
      window.location.assign("/")
    }
    handleChange(event){
      this.setState({
          [event.target.name] : event.target.value
      })
    }

  render() {
    return (
        <form onSubmit={this.handleSubmit}>
            <h3><label>{this.props.queue}</label></h3>
            <p><label>Queue name:  <input name="queue_name" type="text" defaultValue={this.props.name} onChange={this.handleChange}/> </label></p>
            <p><label>It's a filter <input name="is_filter" type="checkbox" defaultChecked={this.props.is_filter} onChange={this.handleChange}/> </label></p>
            <p><label>Query/filter text <textarea name="query_text" defaultValue={this.props.query_text} style={{width: 400 + "px"}} onChange={this.handleChange}/> </label></p>
            <p><label>Yellow limit(tickets) <input name="ts_warning_limit" type="text" defaultValue={this.props.ts_warning_limit} onChange={this.handleChange}/> </label></p>
            <p><label>Red limit(tickets) <input name="ts_danger_limit" type="text" defaultValue={this.props.ts_danger_limit} onChange={this.handleChange}/> </label></p>
            <p><label>Yellow limit(crew) <input name="crew_warning_limit" type="text" defaultValue={this.props.crew_warning_limit} onChange={this.handleChange}/></label> </p>
            <p><label>Red limit(crew) <input name="crew_danger_limit" type="text" defaultValue={this.props.crew_danger_limit} onChange={this.handleChange}/> </label></p>
            {//<p><label>Assignees <input name="assignees" type="text" defaultValue={this.props.assignees}
               //                         onChange={this.handleChange}/> </label></p>
            }
            <select name="assignees" onChange={(e)=>this.handleMultChange(e.target.selectedOptions)} multiple={true} defaultValue={this.props.assignees}>
                {this.props.s_units.map((item)=><option
                value={item.login}>{item.login}</option>)}
            </select>

            <p><button className="btn btn-primary" type="submit">Submit</button> </p>
          </form>
    )
  }
}

export  default AssigneeEditorForm;
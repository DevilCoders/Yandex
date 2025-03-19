import React, {useEffect, useState} from "react";
import axios from "axios";
import { library } from '@fortawesome/fontawesome-svg-core'
import {faEdit, fas, faUser, faPlus, faMinus} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {Link} from "react-router-dom";


const Queue = ({queue, deleteQueue, units_absent, units_pending, user_is_auth}) => {
    library.add(fas, faUser, faEdit, faPlus, faMinus)

    const [tickets_open, setTicketsOpen] = useState(0)
    const [tickets_in_progress, setTicketsInProgress] = useState(0)
    const [tickets_sla_failed, setTicketsSlaFailed] = useState(0)
    const [mouseMove, checkMouseMove] = useState(0)

    function tracked_event(ev) { checkMouseMove(ev.pageX + ev.pageY); }

    var queue_endpoint = 'http://localhost:8000/api/ext'
    var headers = {
        'Content-Type': 'application/json'

    }
    useEffect(() => {
              axios.post(queue_endpoint, {tickets_open: queue.ts_open, req_type: queue.query_type}, {headers: headers})
                   .then(response => {
                        setTicketsOpen(response.data['tickets_open'])
                        setTicketsInProgress(response.data['tickets_in_progress'])
                        setTicketsSlaFailed(response.data['tickets_sla_failed'])

                   })
                   .catch((error) => (console.log(error)))
    }, [mouseMove])

    let queue_actions = ''
    if (user_is_auth) {
        queue_actions =  <td>
                    <Link to={`edit-assignees/${queue.q_id}`}>
                        <FontAwesomeIcon icon={faEdit}>Nothing - why?</FontAwesomeIcon>
                    </Link>
                    <Link to={`new-queue/${queue.q_id}`}>
                        <FontAwesomeIcon icon={faPlus}>Nothing - why?</FontAwesomeIcon>
                    </Link>
                    <button type="button" onClick={() => deleteQueue(queue.q_id)}>
                        <FontAwesomeIcon icon={faMinus}>Nothing - why?</FontAwesomeIcon>
                    </button>
                </td>
    }

    const assignees = []
    for (let unit in queue.assignees)
    {
        let dedup = 0
        for (let pending in units_pending)
        {
            if ((units_pending[pending]['login'] === queue.assignees[unit]) & (units_pending[pending]['q_name'] === queue.name))
            {
                assignees.push(queue.assignees[unit] + '(?)')
                dedup = 1
            }

        }
            if (dedup === 0) {
                assignees.push(queue.assignees[unit])
            }
    }
    return(

        <tr onMouseMove={(ev)=> tracked_event(ev)}>
            <td>{queue.name}</td>
            <td>{queue.crew_danger_limit}</td>
            <td>{queue.crew_warning_limit}</td>
            <td>{queue.ts_danger_limit}</td>
            <td>{queue.ts_warning_limit}</td>
            <td>{tickets_open}</td>
            <td>{tickets_in_progress}</td>
            <td>{tickets_sla_failed}</td>
            <td>{assignees.map((assignee) => {

                if (units_absent.includes(assignee))
                {
                    assignee = <span style={{color: "red"}}> {assignee} </span>
                }
                else
                {
                    assignee = assignee + ' '
                }

                return assignee
            })}</td>

            {queue_actions}

        </tr>
    )
}

export default Queue;
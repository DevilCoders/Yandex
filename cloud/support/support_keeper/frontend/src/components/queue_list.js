import React from "react";
import Queue from "./queue";

const QueueList = ({queues, deleteQueue, units_absent, units_pending, user_is_auth}) => {
    let actions = ''
    if (user_is_auth)
    {
        actions = <th>Actions</th>
    }
        return(

        <table className="table table-striped">
            <thead><tr>
                <th>Queue name</th>
                <th>min Crew</th>
                <th>Crew</th>
                <th>Max tickets</th>
                <th>Many tickets</th>
                <th>Tickets Open</th>
                <th>Tickets in Progress</th>
                <th>SLA failed in</th>
                <th>Persons assigned</th>
                {actions}
            </tr>
            </thead>
            <tbody>
            {queues.map((queue) => (<Queue
                queue={queue}
                deleteQueue={deleteQueue}
                units_absent={units_absent}
                units_pending={units_pending}
                user_is_auth = {user_is_auth}
            />))}
            </tbody>
        </table>
    )
}

export default QueueList;

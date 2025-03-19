import {useParams} from "react-router-dom";
import AssigneeEditorForm from "./AssigneeEditorForm"

const QueueForm = ({queues, callback, s_units, is_auth}) => {
        let id = useParams()
        let filtered_queues = queues.filter((q) => q.q_id === id.id)
        return(
            <AssigneeEditorForm queue = {filtered_queues[0].q_id}
                                is_filter = {filtered_queues[0].query_type === 'filter'}
                                query_text ={filtered_queues[0].ts_open}
                                name = {filtered_queues[0].name}
                                ts_warning_limit = {filtered_queues[0].ts_warning_limit}
                                ts_danger_limit = {filtered_queues[0].ts_danger_limit}
                                crew_warning_limit = {filtered_queues[0].crew_warning_limit}
                                crew_danger_limit = {filtered_queues[0].crew_danger_limit}
                                assignees = {filtered_queues[0].assignees}
                                callback = {callback}
                                s_units = {s_units}
                                is_auth = {is_auth}
            />
        )
}

export default QueueForm;
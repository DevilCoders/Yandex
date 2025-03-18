import '../css/FailedTaskCard.css'
import PopUpTraceback from "./PopUpTraceback";
import React from "react";


const FailedTaskCard = props => {
    let args = props.task_info.args
    let kwargs = props.task_info.kwargs
    let content = props.task_info.traceback

    let caption = props.task_info.failed_count
    let class_name = 'FailedTaskCard'
    if (!caption) {
        caption = props.task_info.retry_count
        class_name = 'RetryTaskCard'
    }

    caption = props.caption ? props.caption : caption

    return <div className={class_name}>
        <PopUpTraceback
            title='Traceback'
            caption={caption}
            args={args}
            kwargs={kwargs}
            content={content}
        />
    </div>
}

export default FailedTaskCard;

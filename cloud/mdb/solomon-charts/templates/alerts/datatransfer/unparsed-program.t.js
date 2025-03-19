let rows = {project='<< datatransfer_project_id >>', name='publisher.data.unparsed_rows', resource_id='<< datatransfer_val >>'};
let count = group_lines('sum', diff(rows));
let sumCount = sum(count);

let is_yellow = sumCount > 100;
let is_red = sumCount > 400;

alarm_if(is_red);
warn_if(is_yellow);

let rows = {project='<< datatransfer_project_id >>', name='publisher.data.parsed_rows', resource_id='<< datatransfer_val >>'};
let count = group_lines('sum', diff(rows));
let sumCount = sum(count);

let is_red = sumCount <= 0;

alarm_if(is_red);

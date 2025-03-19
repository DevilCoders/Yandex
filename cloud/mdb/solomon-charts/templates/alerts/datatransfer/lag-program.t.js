let lag = series_sum({project='<< datatransfer_project_id >>', resource_id='<< datatransfer_val >>', name='sinker.pusher.time.row_max_lag_sec'});

let result = percentile(90, lag);
let reason = 'lag: ' + to_fixed(result, 0) + ' seconds';
alarm_if(result > 600);
warn_if(result > 300);

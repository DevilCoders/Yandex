import { formatDistance } from 'date-fns'


function DiffDate(dt) {
    return !dt ? null : formatDistance(dt, new Date(), { addSuffix: true })
}

export default DiffDate;

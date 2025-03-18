export const DEFAULT_TAG = 'default';

function getProfileTags() {
    let data = {};

    try {
        data = JSON.parse(localStorage.getItem('sbs-profile-tag') || '{}');
    } catch (e) {}

    return data;
}

export function getProfileTag(serviceId) {
    const data = getProfileTags();

    return data[serviceId] || DEFAULT_TAG;
}

export function setProfileTag(serviceId, val) {
    let data = getProfileTags();

    data[serviceId] = val;
    localStorage.setItem('sbs-profile-tag', JSON.stringify(data));
}

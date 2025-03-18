let user = null;

function hasPermission(permission) {
    return user.permissions.includes(permission);
}

export function setUser(userObject) {
    user = {
        ...userObject,
        hasPermission
    };
}

export function getUser() {
    return user;
}

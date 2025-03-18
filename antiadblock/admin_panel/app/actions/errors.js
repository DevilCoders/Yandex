export const SET_ERRORS = 'SET_ERRORS';
export const CLEAR_ERRORS = 'CLEAR_ERRORS';

export function setGlobalErrors(errors) {
    return setErrors('global', errors);
}

export function clearGlobalErrors() {
    return clearErrors('global');
}

export function setErrors(id, errors) {
    return {
        type: SET_ERRORS,
        id,
        errors
    };
}

export function clearErrors(id) {
    return {
        type: CLEAR_ERRORS,
        id
    };
}

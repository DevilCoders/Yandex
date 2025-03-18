import {Response, Headers, Request} from 'whatwg-fetch';

global.fetch = require('jest-fetch-mock');

global.Response = Response;
global.Headers = Headers;
global.Request = Request;

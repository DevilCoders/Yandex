#!/usr/bin/env python

import cgi
import MySQLdb
import xml.dom.minidom as minidom

import config
from db import DBWorker

class Request:
    def __init__(self, storage, fields):
        for field in fields:
            value = None
            if field in storage:
                value = storage[field].value
            setattr(self, field, value)

class Response:
    factory = minidom.Document()

    def __init__(self, status="ok", message=None):
        self.root = self.factory.createElement("response")
        self.root.setAttribute("status", status)
        if message:
            self.root.appendChild(self.factory.createTextNode(message))

    def addPoint(self, **kws):
        pointElement = self.factory.createElement("point")
        for key in kws:
            pointElement.setAttribute(key, kws[key])
        self.root.appendChild(pointElement)

    def __str__(self):
        return self.root.toxml("utf-8")

class Application:
    def __init__(self, config):
        self.db = DBWorker(config)

    def processRequest(self):
        storage = cgi.FieldStorage()
        request = Request(storage, ["action", "name", "date", "serp", "value", "size"])
        if request.date:
            request.date = request.date.replace('-', '_')
        response = Response("error", "unknown action")
        if request.action == "get":
            response = self.processGet(request)
        elif request.action == "set":
            response = self.processSet(request)
        self.printResponse(response)

    def processGet(self, request):
        if not request.date or not request.serp:
            return Response("error", "not enought data")
        cursor = self.db.getCursor()
        cursor.execute("select name, value, size from metrics where date=%s and serp=%s", (request.date, request.serp))
        response = Response()
        for row in cursor:
            response.addPoint(system=row[0], value=row[1], size=row[2])
        return response

    def processSet(self, request):
        if not request.date or not request.serp or not request.name \
                            or not request.value or not request.size:
            return Response("error", "not enought data")
        cursor = self.db.getCursor()
        cursor.execute("delete from metrics where name=%s and date=%s and serp=%s", (request.name, request.date, request.serp))
        cursor.execute("insert into metrics values(%s, %s, %s, %s, %s)", (request.name, request.date, request.serp, request.value, request.size))
        return Response()

    def printResponse(self, response):
         print """Content-Type: text/xml\n"""
         print """<?xml version="1.0" encoding="utf-8" ?>"""
         print response


if __name__ == "__main__":
    Application(config).processRequest()

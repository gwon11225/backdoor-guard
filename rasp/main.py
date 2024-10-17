import flask
from datetime import datetime
import serial
import json
import time

class OutsideTime:
    def __init__(self, id, title, start, end, week):
        self.id = id
        self.title = title
        self.start = start
        self.end = end
        self.week = week
        
    def to_dict(self):
        return {
            'id': self.id,
            'title': self.title,
            'start': self.start.strftime(datetime_format),
            'end': self.end.strftime(datetime_format),
            'week': self.week
        }
    
    def to_dict_arduino(self):
        return str(self.week) + ',' + self.start.strftime(datetime_format) + ',' + self.end.strftime(datetime_format) + '\n'

app = flask.Flask(__name__)

timeList = []

autoincrement = 0
datetime_format = '%H:%M:%S'


ser = serial.Serial("/dev/ttyS0", baudrate=9600, timeout=1)

def sendTimeListToArduino():
    ser.write('\r'.encode('utf-8'))
    for i in timeList:
        print(i.to_dict_arduino())
        ser.write(i.to_dict_arduino().encode('utf-8'))

@app.route("/time", methods=['GET'])
def getTimes():
    return flask.jsonify([t.to_dict() for t in timeList])

@app.route("/time", methods = ['POST'])
def creatTime():
    global autoincrement
    request = flask.request.get_json()
    
    title: str = request.get('title')
    starttime: str = request.get('start')
    endtime: str = request.get('end')
    week: str = int(request.get('week'))
    
    start = datetime.strptime(starttime, datetime_format).time()
    end = datetime.strptime(endtime, datetime_format).time()
    
    timeList.append(OutsideTime(autoincrement, title, start, end, week))
    
    autoincrement += 1

    sendTimeListToArduino()

    return {'message' : 'success'}

@app.route("/time", methods = ['DELETE'])
def deleteTime():
    id = int(flask.request.args.get('id'))
    
    for i in range(len(timeList)):
        if timeList[i].id == id:
            timeList.pop(i)
            break

    print(timeList)
        
    sendTimeListToArduino()    
        
    return {'message' : 'success'}

app.run(host='0.0.0.0')

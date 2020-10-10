#!/bin/usr/env python

from m5.params import *
from m5.SimObject import SimObject

class MyHelloObject(SimObject):
  type = 'MyHelloObject'
  cxx_header = 'learning_gem5/gem5_tut/my_hello_object.hh'

  time_to_wait     = Param.Latency('1us', 'Time gefore firing the event')
  number_of_events = Param.Int(10, 'Number of times that the event occure')
  good_bye         = Param.MyGoodByeObject('A good bye object')
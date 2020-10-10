#!/usr/bin/env python

from m5.params import *
from m5.SimObject import SimObject

class SimpleMemObj(SimObject):
  type       = 'SimpleMemObj'
  cxx_header = 'learning_gem5/simple_mem_obj/SimpleMemObj.hh'
  
  inst_port = ResponsePort('CPU side port, receives requests')
  data_port = ResponsePort('CPU side port, receives requests')
  mem_port  = RequestPort('Memory side port, sends requests')
#!/usr/bin/env python

from m5.params import *
from m5.SimObject import SimObject

class MyGoodByeObject(SimObject):
  type = 'MyGoodByeObject'
  cxx_header = 'learning_gem5/gem5_tut/my_good_bye_object.hh'

  buffer_size = Param.MemorySize('1kB', 'Size of buffer to fill with good bye')
  write_bandwidth = Param.MemoryBandwidth('100MB/s', 'Bandwidth to fill the buffer')
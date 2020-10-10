#pragma once

#include "params/MyGoodByeObject.hh"
#include "sim/sim_object.hh"
#include "sim/eventq.hh"
#include <atomic>
#include <string>

class MyGoodByeObject : public SimObject
{
  public:
    MyGoodByeObject(MyGoodByeObjectParams* params);
    ~MyGoodByeObject();
    void sayGoodBye(std::string name);
  private:
    void processEvent();
    void fillBuffer();
    EventWrapper<MyGoodByeObject, &MyGoodByeObject::processEvent> event_;
    float       bandwidth_;
    int         buffer_size_;
    char*       buffer_;
    int         buffer_last_used_;
    std::string message_;
};  
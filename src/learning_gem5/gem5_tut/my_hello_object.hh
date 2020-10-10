#pragma once

#include "params/MyHelloObject.hh"
#include "sim/sim_object.hh"
#include "sim/eventq.hh"
#include <atomic>
#include <string>

class MyGoodByeObject;

class MyHelloObject : public SimObject
{
  public:
    MyHelloObject(MyHelloObjectParams* p);
    ~MyHelloObject();
    void startup();
  private:
    void processEvent();
    EventWrapper<MyHelloObject, &MyHelloObject::processEvent> event_;
    const std::string name_;
    const Tick latency_;
    std::atomic<int> times_left_;
    MyGoodByeObject* const good_bye_;
};
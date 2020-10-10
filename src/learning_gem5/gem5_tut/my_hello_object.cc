#include "learning_gem5/gem5_tut/my_hello_object.hh"
#include "learning_gem5/gem5_tut/my_good_bye_object.hh"
#include "base/trace.hh"
#include "debug/MyHello.hh"

MyHelloObject::MyHelloObject(MyHelloObjectParams* params) :
  SimObject(params),
  event_(*this),
  name_(params->name),
  latency_(params->time_to_wait),
  times_left_(params->number_of_events),
  good_bye_(params->good_bye)
{
  DPRINTF(MyHello, "MyHelloObject::ctor\n");
  panic_if(!good_bye_, "MyHelloObject needs to have a MyGoodByeObject");
}

MyHelloObject::~MyHelloObject()
{
  DPRINTF(MyHello, "MyHelloObject::dtor\n");
}

MyHelloObject* MyHelloObjectParams::create()
{
  return new MyHelloObject(this);
}

void MyHelloObject::processEvent()
{
  if(--times_left_)
  {
    DPRINTF(MyHello, "MyHelloObject::processEvent: schedule again\n");
    schedule(event_, curTick() + latency_);
  }
  else
  {
    DPRINTF(MyHello, "MyHelloObject::processEvent: done\n");
    good_bye_->sayGoodBye(name_);
  }
}

void MyHelloObject::startup()
{
  schedule(event_, latency_);
}
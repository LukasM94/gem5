#include "learning_gem5/gem5_tut/my_good_bye_object.hh"
#include "base/trace.hh"
#include "sim/sim_exit.hh"
#include "debug/MyHello.hh"

MyGoodByeObject::MyGoodByeObject(MyGoodByeObjectParams* params) :
  SimObject(params),
  event_(*this),
  bandwidth_(params->write_bandwidth),
  buffer_size_(params->buffer_size),
  buffer_(new char[buffer_size_]),
  buffer_last_used_(0),
  message_()
{
  DPRINTF(MyHello, "MyGoodByeObject::ctor\n");
}

MyGoodByeObject::~MyGoodByeObject()
{
  delete buffer_;
  DPRINTF(MyHello, "MyGoodByeObject::dtor\n");
}

MyGoodByeObject* MyGoodByeObjectParams::create()
{
  return new MyGoodByeObject(this);
}

void MyGoodByeObject::sayGoodBye(std::string name)
{
  // DPRINTF(MyHello, "MyGoodByeObject::sayGoodBye: name <%s>\n", name.c_str());
  message_ = "Good bye to " + name + ".";
  fillBuffer();
}

void MyGoodByeObject::processEvent()
{
  DPRINTF(MyHello, "MyGoodByeObject::processEvent\n");
  fillBuffer();
}

void MyGoodByeObject::fillBuffer()
{
  DPRINTF(MyHello, "MyGoodByeObject::fillBuffer\n");
  assert(message_.length() > 0);

  int        copied = 0;
  auto       it     = message_.begin();
  const auto end    = message_.end();
  while (it != end && buffer_last_used_ < buffer_size_)
  {
    buffer_[buffer_last_used_] = *it;

    ++buffer_last_used_;
    ++copied;
    ++it;
  }

  int ticks = bandwidth_ * copied;
  if (buffer_last_used_ < buffer_size_ - 1)
  {
    DPRINTF(MyHello, "MyGoodByeObject::fillBuffer: schedule in %d ticks\n", ticks);
    schedule(event_, curTick() + ticks);
  }
  else
  {
    DPRINTF(MyHello, "MyGoodByeObject::fillBuffer: done with filling the buffer\n");
    DDUMP(MyHello, buffer_, buffer_size_);
    exitSimLoop(buffer_, 0, curTick() + ticks);
  }
}

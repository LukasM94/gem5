#include "params/SimpleMemObj.hh"
#include "learning_gem5/simple_mem_obj/SimpleMemObj.hh"
#include "debug/SimpleMemObj.hh"
#include "base/trace.hh"

const char SimpleMemObj::NAME_INST_PORT[] = "inst_port";
const char SimpleMemObj::NAME_DATA_PORT[] = "data_port";
const char SimpleMemObj::NAME_MEM_PORT[] = "mem_port";

/*
 * Constructor for SimpleMemOb.
 * 
 * @param params the python variable which initialize this 
 *               object
 */
SimpleMemObj::SimpleMemObj(SimpleMemObjParams* params) :
  SimObject(params),
  blocked_(false),
  inst_port_(params->name + "." + NAME_INST_PORT, this),
  data_port_(params->name + "." + NAME_DATA_PORT, this),
  mem_port_(params->name + "." + NAME_MEM_PORT, this)
{
  DPRINTF(SimpleMemObj, "SimpleMemObj::ctor\n");
}

/*
 * Called whene trying to connect a slave port to this object.
 * 
 * @param if_name python variable name of the interface for this
 *                object
 * @param idx is the port number when using vector ports and is
 *            'InvalidPortID' by default
 * @return a reference to the corresponding port
 */
Port& SimpleMemObj::getPort(const std::string& if_name, PortID idx)
{
  DPRINTF(SimpleMemObj, "SimpleMemObj::getPort: if_name <%s>, idx <%d>\n", if_name.c_str(), idx);
  panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

  if (if_name.compare(NAME_MEM_PORT) == 0)
  {
    return mem_port_;
  }
  else if (if_name.compare(NAME_INST_PORT) == 0)
  {
    return inst_port_;
  }
  else if (if_name.compare(NAME_DATA_PORT) == 0)
  {
    return data_port_;
  }
  else 
  {
    return SimObject::getPort(if_name, idx);
  }
}

/*
 * Returns a list of non overlapping address ranges the owner is 
 * responsible for. This is used by the crossbar objects to know 
 * which port to send requests to. Most memory objects will either 
 * return 'AllMemory' or return whateer address range their peer 
 * responds to.
 * 
 * @return list of address range
 */
AddrRangeList SimpleMemObj::CPUSidePort::getAddrRanges() const
{
  return owner_->getAddrRanges();
}

/*
 * Is called whenever the cpu tries to make an atomic memory 
 * access. Not implemented.
 */
Tick SimpleMemObj::CPUSidePort::recvAtomic(Packet* pkt)
{
  panic("recvAtomic unimpl.");
}

/*
 * Called when the cpu makes a functional access. Performs a 
 * 'debug' access updating/reading the data in place.
 * 
 * @param packet
 */
void SimpleMemObj::CPUSidePort::recvFunctional(Packet* pkt)
{
  owner_->handleFunctional(pkt);
}

/*
 * Called when the peer to this port calls 'sendTimingReq'. It
 * takes a single parameter which is the packet pointer for the
 * request. This function returns true if the packet is 
 * accepted. If this function return false, at some point in the 
 * future this object must call 'sendReqRetry' so notify the peer 
 * port that it is able to accepted the rejected request.
 * 
 * @param packet
 * @return true if the packet is accepted
 *         false if at some point in the future this object must
 *         call 'sendReqRetry' so notfify the peer port that it
 *         is able to accepted the rejected request
 */
bool SimpleMemObj::CPUSidePort::recvTimingReq(Packet* pkt)
{
  if (!owner_->handleRequest(pkt))
  {
    need_retry_ = true;
    return false;
  }
  else
  {
    return true;
  }
}

/*
 * Called when the peer port calls 'sendRespRetry'. When this 
 * function is executed, this port should call 'sendTimingResp' 
 * again to retry sending the response to its peer master port.
 */
void SimpleMemObj::CPUSidePort::recvRespRetry()
{
  assert(blocked_packet_ != nullptr);

  Packet* pkt   = blocked_packet_;
  blocked_packet_ = nullptr;

  sendPacket(pkt);
}

/*
 * Send a packet across this port. This is called by the owner
 * and all the flow control is handled in this function.
 * 
 * @param packet
 */
void SimpleMemObj::CPUSidePort::sendPacket(Packet* pkt)
{
  panic_if(blocked_packet_ != nullptr, "Should never try to send if blocked_!");

  if (!sendTimingResp(pkt))
  {
    blocked_packet_ = pkt;
  }
}

/*
 * Send a retry to the peer port if it is neede. This is called 
 * from the SimpleMemObj whenever it is unblocked.
 */
void SimpleMemObj::CPUSidePort::trySendRetry()
{
  if (need_retry_ && blocked_packet_ == nullptr)
  {
    need_retry_ = false;
    DPRINTF(SimpleMemObj, "SimpleMemObj::CPUSidePort::trySendRetry\n");
    sendRetryReq();
  }
}

/*
 * Called when the slave peer to this port calls 'sendTimingResp'.
 * 
 * @param packet
 * @return true if this object can accept the response
 *         false if at some point in the future this object must
 *         call 'sendRespRetry' to notify its peer that it is now
 *         capable of receiving the response
 */
bool SimpleMemObj::MemSidePort::recvTimingResp(Packet* pkt)
{
  return owner_->handleResponse(pkt);
}

/*
 * Called when the peer port calls 'sendReqRetry' and means this
 * object should try resending a packet that previously failed.
 */
void SimpleMemObj::MemSidePort::recvReqRetry()
{
  assert(blocked_packet_ != nullptr);

  Packet* pkt     = blocked_packet_;
  blocked_packet_ = nullptr;

  sendPacket(pkt);
}

/*
 * Similar to 'sendRaneChange' above, this function is called 
 * whenever the peer port wants to notify this object that the
 * address ranges it accepts are changing. This function normally 
 * is only called during the initialization of the memory system 
 * and not while the simulation is executing.
 */
void SimpleMemObj::MemSidePort::recvRangeChange()
{
  owner_->sendRangeChange();
}

/*
 * Send a packet across this port. This is called by the owner 
 * and all of the flow control is hanled in this function.
 * 
 * @param packet
 */
void SimpleMemObj::MemSidePort::sendPacket(Packet* pkt)
{
  panic_if(blocked_packet_ != nullptr, "Shoud never try to send if blocked!");
  if (!sendTimingReq(pkt))
  {
    blocked_packet_ = pkt;
  }
}

/*
 * Handle a packet functionally. Update the data on a write and 
 * get the data on a read.
 * 
 * @param packet
 */
void SimpleMemObj::handleFunctional(Packet* pkt)
{
  mem_port_.sendFunctional(pkt);
}

/*
 * Return the address ranges this SimObj is responisble for. Just
 * same as the next upper level of the hierarchy.
 * 
 * @return list of addres range
 */
AddrRangeList SimpleMemObj::getAddrRanges() const
{
  DPRINTF(SimpleMemObj, "SimpleMemObj::getAddrRanges\n");
  return mem_port_.getAddrRanges();
}

/*
 * Tell the cpu side to ask for our memory ranges.
 */
void SimpleMemObj::sendRangeChange()
{
  DPRINTF(SimpleMemObj, "SimpleMemObj::sendRangeChange\n");
  inst_port_.sendRangeChange();
  data_port_.sendRangeChange();
}

/*
 * Handles the request from the cpu side.
 *
 * @param requested packet
 * @return true if we can handle the request
 *         fale if it is blocked
 */
bool SimpleMemObj::handleRequest(Packet* pkt)
{
  if (blocked_) 
  {
    DPRINTF(SimpleMemObj, "SimpleMemObj::handleRequest: blocked\n");
    return false;
  }
  DPRINTF(SimpleMemObj, "SimpleMemObj::handleRequest: got request for address <%#x>\n", pkt->getAddr());
  blocked_ = true;
  mem_port_.sendPacket(pkt);
  return true;
}

/*
 * Handles the response from the memory side.
 * 
 * @param packet
 * @return true if we can handle this response
 *         false if the responder needs to retry later
 */
bool SimpleMemObj::handleResponse(Packet* pkt)
{
  assert(blocked_);
  DPRINTF(SimpleMemObj, "SimpleMemObj::handleResponse: got response from address <%#x>\n", pkt->getAddr());

  blocked_ = false;

  // Simply forward to the memory port
  if (pkt->req->isInstFetch())
  {
    inst_port_.sendPacket(pkt);
  }
  else
  {
    data_port_.sendPacket(pkt);
  }

  inst_port_.trySendRetry();
  data_port_.trySendRetry();

  return true;
}

SimpleMemObj* SimpleMemObjParams::create()
{
    return new SimpleMemObj(this);
}

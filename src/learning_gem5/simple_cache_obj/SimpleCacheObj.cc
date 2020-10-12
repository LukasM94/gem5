
#include "learning_gem5/simple_cache_obj/SimpleCacheObj.hh"

#include "base/random.hh"
#include "base/trace.hh"
#include "debug/SimpleCacheObj.hh"
#include "params/SimpleCacheObj.hh"
#include "sim/system.hh"

const char SimpleCacheObj::NAME_CPU_PORTS[]  = "cpu_ports";
const char SimpleCacheObj::NAME_MEM_PORT[]   = "mem_port";

SimpleCacheObj::SimpleCacheObj(SimpleCacheObjParams* params) :
  ClockedObject(params),
  latency_(params->latency),
  block_size_(params->system->cacheLineSize()),
  capacity_(params->size / block_size_),
  blocked_(false),
  outstanding_packet_(nullptr),
  waiting_port_id_(-1),
  miss_time_(0),
  stats_(this),
  mem_port_(params->name + "." + NAME_MEM_PORT, this)
{
  DPRINTF(SimpleCacheObj, "SimpleCacheObj::ctor: latency_ <%d> block_size_ \
                        <%d> capacity_ <%d>\n", \
                        latency_, block_size_, capacity_);
  assert(params->port_mem_port_connection_count == 1 && \
         "It is only allowed to have one mem port");
  assert(params->port_cpu_ports_connection_count > 0 && \
         "There must be more than 0 cpu ports");

  for (int i = 0; i < params->port_cpu_ports_connection_count; ++i)
  {
    std::string name = params->name + ".";
    name += NAME_CPU_PORTS;
    name += std::to_string(i);
    cpu_ports_.emplace_back(name, i, this);
  }
}

Port& SimpleCacheObj::getPort(const std::string& if_name, PortID idx)
{
  if (if_name.compare(NAME_CPU_PORTS) == 0)
  {
    assert(idx != InvalidPortID && \
           "Mem side of simple cache not a vector port");
    assert(idx < cpu_ports_.size());
    return cpu_ports_[idx];
  }
  else if (if_name.compare(NAME_MEM_PORT) == 0)
  {
    return mem_port_;
  }
  else
  {
    return SimObject::getPort(if_name, idx);
  }
}

bool SimpleCacheObj::handleRequest(Packet* pkt, int port_id)
{
  bool old_value = true;
  old_value = __atomic_exchange_n(&blocked_, old_value, __ATOMIC_RELAXED);

  if (old_value)
  {
    DPRINTF(SimpleCacheObj, "SimpleCacheObj::handleRequest: blocked_\n");
    return false;
  }

  DPRINTF(SimpleCacheObj, "SimpleCacheObj::handleRequest: addr <%x> port id \
                           <%d>\n", pkt->getAddr(), port_id);

  assert(waiting_port_id_ == -1);
  waiting_port_id_ = port_id;

  schedule(new AccessEvent(this, pkt), clockEdge(latency_));
  return true;
}

void SimpleCacheObj::accessTiming(Packet* pkt)
{
  bool hit = accessFunctional(pkt);

  DPRINTF(SimpleCacheObj, "SimpleCacheObj::accessTiming: %s for packet: %s\n",
                          hit ? "hit" : "miss", pkt->print());

  if (hit)
  {
    stats_.hits_++;
    DDUMP(SimpleCacheObj, pkt->getConstPtr<uint8_t>(), pkt->getSize());
    pkt->makeResponse();
    sendResponse(pkt);
    return;
  }

  stats_.misses_++;
  miss_time_ = curTick();

  Addr         addr       = pkt->getAddr();
  Addr         block_addr = pkt->getBlockAddr(block_size_);
  unsigned int size       = pkt->getSize();

  // Forward to the memory side.
  // We can't directly forward the packet unless it is exactly the size
  // of the cache line, and aligned. Check for that here.
  if (addr == block_addr && size == block_size_)
  {
    DPRINTF(SimpleCacheObj, "SimpleCacheObj::accessTiming: forwarding \
                             packet\n");
    mem_port_.sendPacket(pkt);
    return;
  }

  DPRINTF(SimpleCacheObj, "SimpleCacheObj::accessTiming: upgrading packet to \
                           block size\n");

  assert(size < block_size_ && \
         "Cannot handle accesses that span multiple cache lines");
  assert(pkt->needsResponse() && "Packet need a response");
  assert((pkt->isWrite() || pkt->isRead()) && \
          "Packet needs to be either reading or writing");

  MemCmd    cmd     = MemCmd::ReadReq;
  PacketPtr new_pkt = new Packet(pkt->req, cmd, block_size_);
  new_pkt->allocate();

  assert(new_pkt->getAddr() == new_pkt->getBlockAddr(block_size_));

  outstanding_packet_ = pkt;

  DPRINTF(SimpleCacheObj, "SimpleCacheObj::accessTiming: forwarding new \
                           packet\n");
  mem_port_.sendPacket(new_pkt);
}

bool SimpleCacheObj::accessFunctional(Packet* pkt)
{
  Addr block_address = pkt->getBlockAddr(block_size_);
  auto it = storage_.find(block_address);
  if (it == storage_.end())
  {
    // miss
    return false;
  }

  // hit
  if (pkt->isWrite())
  {
    pkt->writeDataToBlock(it->second, block_size_);
  }
  else if (pkt->isRead())
  {
    pkt->setDataFromBlock(it->second, block_size_);
  }
  else
  {
    assert(false && "SimpleCacheObj::accessFunctional: pkt neither read nor \
                     write");
  }
  return true;
}

void SimpleCacheObj::sendResponse(Packet* pkt)
{
  DPRINTF(SimpleCacheObj, "SimpleCacheObj::sendResponse: sending resp for \
                           addr %#x\n", pkt->getAddr());
  int port = waiting_port_id_;

  waiting_port_id_ = -1;

  cpu_ports_[port].sendPacket(pkt);
  for (auto& cpu_port : cpu_ports_)
  {
    cpu_port.trySendRetry();
  }

  bool old_value = false;
  old_value = __atomic_exchange_n(&blocked_, old_value, __ATOMIC_RELAXED);
  assert(old_value && "SimpleCacheObj::sendResponse: blocked_ \
                       should be true till response is sent");
}

bool SimpleCacheObj::handleResponse(Packet* pkt)
{
  assert(blocked_ && "Should be blocked");
  DPRINTF(SimpleCacheObj, "SimpleCacheObj::handleResponse: addr <%x>\n", \
                          pkt->getAddr());

  insert(pkt);
  stats_.miss_latency_.sample(curTick() - miss_time_);

  if (outstanding_packet_ != nullptr)
  {
    DPRINTF(SimpleCacheObj, "SimpleCacheObj::handleResponse: copying data \
                             from new packet to old\n");
    __attribute__((unused))bool hit = accessFunctional(outstanding_packet_);
    assert(hit && "Should alfays hit after inserting");
    outstanding_packet_->makeResponse();

    delete pkt;

    pkt                 = outstanding_packet_;
    outstanding_packet_ = nullptr;
  }

  sendResponse(pkt);

  return true;
}

void SimpleCacheObj::insert(Packet* pkt)
{
  assert(pkt->getAddr() ==  pkt->getBlockAddr(block_size_) && \
         "The packet should be aligned");
  assert(storage_.find(pkt->getAddr()) == storage_.end() && \
         "The address should not be in the cache");
  assert(pkt->isResponse() && "The packet should be a response");

  if (storage_.size() >= capacity_)
  {
    // Select random thing to evict. This is a little convoluted since we
    // are using a std::unordered_map. See http://bit.ly/2hrnLP2
    int bucket, bucket_size;
    do {
      bucket = random_mt.random(0, (int)storage_.bucket_count() - 1);
    } while ( (bucket_size = storage_.bucket_size(bucket)) == 0 );
    auto block = std::next(storage_.begin(bucket), \
                           random_mt.random(0, bucket_size - 1));

    DPRINTF(SimpleCacheObj, "Removing addr %#x\n", block->first);

    // Write back the data.
    // Create a new request-packet pair
    auto req = std::make_shared<Request>(block->first, block_size_, 0, 0);

    Packet* new_pkt = new Packet(req, MemCmd::WritebackDirty, block_size_);
    new_pkt->dataDynamic(block->second); // This will be deleted later

    DPRINTF(SimpleCacheObj, "Writing packet back %s\n", pkt->print());
    // Send the write to memory
    mem_port_.sendPacket(new_pkt);

    // Delete this entry
    storage_.erase(block->first);
  }

  DPRINTF(SimpleCacheObj, "Inserting %s\n", pkt->print());
  DDUMP(SimpleCacheObj, pkt->getConstPtr<uint8_t>(), block_size_);

  // Allocate space for the cache block block
  uint8_t* block = new uint8_t[block_size_];

  // Insert the block and address into the cache store
  storage_[pkt->getAddr()] = block;

  // Write the block into the cache
  pkt->writeDataToBlock(block, block_size_);
}

/*
 * Tell the cpu side to ask for our memory ranges.
 */
void SimpleCacheObj::sendRangeChange() const
{
  DPRINTF(SimpleCacheObj, "SimpleCacheObj::sendRangeChange\n");
  for (auto& cpu_port : cpu_ports_)
  {
    cpu_port.sendRangeChange();
  }
}

/*
 * Return the address ranges this SimObj is responisble for. Just
 * same as the next upper level of the hierarchy.
 *
 * @return list of addres range
 */
AddrRangeList SimpleCacheObj::getAddrRanges() const
{
  DPRINTF(SimpleCacheObj, "SimpleCacheObj::getAddrRanges\n");
  return mem_port_.getAddrRanges();
}

/*
 * Handle a packet functionally. Update the data on a write and
 * get the data on a read.
 *
 * @param packet
 */
void SimpleCacheObj::handleFunctional(Packet* pkt)
{
  if (accessFunctional(pkt))
  {
    pkt->makeResponse();
  }
  else
  {
    mem_port_.sendFunctional(pkt);
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
AddrRangeList SimpleCacheObj::CPUSidePort::getAddrRanges() const
{
  return owner_->getAddrRanges();
}

/*
 * Is called whenever the cpu tries to make an atomic memory
 * access. Not implemented.
 */
Tick SimpleCacheObj::CPUSidePort::recvAtomic(Packet* pkt)
{
  panic("recvAtomic unimpl.");
}

/*
 * Called when the cpu makes a functional access. Performs a
 * 'debug' access updating/reading the data in place.
 *
 * @param packet
 */
void SimpleCacheObj::CPUSidePort::recvFunctional(Packet* pkt)
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
bool SimpleCacheObj::CPUSidePort::recvTimingReq(Packet* pkt)
{
  DPRINTF(SimpleCacheObj, "SimpleCacheObj::CPUSidePort::recvTimingReq: got \
                           request %s\n", pkt->print());
  if (blocked_packet_ || need_retry_)
  {
    DPRINTF(SimpleCacheObj, "SimpleCacheObj::CPUSidePort::recvTimingReq: \
                             request blocked\n");
    need_retry_ = true;
    return false;
  }
  // Just forward to the cache.
  if (!owner_->handleRequest(pkt, port_number_))
  {
    DPRINTF(SimpleCacheObj, "SimpleCacheObj::CPUSidePort::recvTimingReq: \
                             request failed\n");
    need_retry_ = true;
    return false;
  }
  DPRINTF(SimpleCacheObj, "SimpleCacheObj::CPUSidePort::recvTimingReq: \
                           succeeded\n");
  return true;
}

/*
 * Called when the peer port calls 'sendRespRetry'. When this
 * function is executed, this port should call 'sendTimingResp'
 * again to retry sending the response to its peer master port.
 */
void SimpleCacheObj::CPUSidePort::recvRespRetry()
{
  Packet* pkt = nullptr;
  pkt = __atomic_exchange_n(&blocked_packet_, pkt, __ATOMIC_RELAXED);

  assert(pkt != nullptr && "blocked_packet_ was nullptr");
  DPRINTF(SimpleCacheObj, "SimpleCacheObj::CPUSidePort::recvRespRetry: \
                           retrying response pkt %s\n", pkt->print());

  sendPacket(pkt);

  trySendRetry();
}

/*
 * Send a packet across this port. This is called by the owner
 * and all the flow control is handled in this function.
 *
 * @param packet
 */
void SimpleCacheObj::CPUSidePort::sendPacket(Packet* pkt)
{
  assert(blocked_packet_ == nullptr && "Should never try to send if \
                                        blocked_!");

  DPRINTF(SimpleCacheObj, "SimpleCacheObj::CPUSidePort::sendPacket: sending \
                           %s to CPU\n", pkt->print());
  if (!sendTimingResp(pkt))
  {
    DPRINTF(SimpleCacheObj, "SimpleCacheObj::CPUSidePort::sendPacket: \
                             failed\n");
    blocked_packet_ = pkt;
  }
}

/*
 * Send a retry to the peer port if it is neede. This is called
 * from the SimpleCacheObj whenever it is unblocked.
 */
void SimpleCacheObj::CPUSidePort::trySendRetry()
{
  if (need_retry_ && blocked_packet_ == nullptr)
  {
    need_retry_ = false;
    DPRINTF(SimpleCacheObj, "SimpleCacheObj::CPUSidePort::trySendRetry\n");
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
bool SimpleCacheObj::MemSidePort::recvTimingResp(Packet* pkt)
{
  return owner_->handleResponse(pkt);
}

/*
 * Called when the peer port calls 'sendReqRetry' and means this
 * object should try resending a packet that previously failed.
 */
void SimpleCacheObj::MemSidePort::recvReqRetry()
{
  Packet* pkt = nullptr;
  pkt = __atomic_exchange_n(&blocked_packet_, pkt, __ATOMIC_RELAXED);
  assert(pkt != nullptr && "blocked_packet_ was nullptr");

  sendPacket(pkt);
}

/*
 * Similar to 'sendRaneChange' above, this function is called
 * whenever the peer port wants to notify this object that the
 * address ranges it accepts are changing. This function normally
 * is only called during the initialization of the memory system
 * and not while the simulation is executing.
 */
void SimpleCacheObj::MemSidePort::recvRangeChange()
{
  owner_->sendRangeChange();
}

/*
 * Send a packet across this port. This is called by the owner
 * and all of the flow control is hanled in this function.
 *
 * @param packet
 */
void SimpleCacheObj::MemSidePort::sendPacket(Packet* pkt)
{
  panic_if(blocked_packet_ != nullptr, "Shoud never try to send if blocked_!");
  if (!sendTimingReq(pkt))
  {
    blocked_packet_ = pkt;
  }
}

SimpleCacheObj::SimpleCacheStats::SimpleCacheStats(Stats::Group* parent) :
  Stats::Group(parent),
  ADD_STAT(hits_, "Number of hits"),
  ADD_STAT(misses_, "Number of misses"),
  ADD_STAT(miss_latency_, "Ticks for misses to the cache"),
  ADD_STAT(hitRatio_, "The ratio of hits to the total accesses to the cache",
                      hits_ / (hits_ + misses_))
{
  miss_latency_.init(16); // number of buckets
}


SimpleCacheObj* SimpleCacheObjParams::create()
{
  return new SimpleCacheObj(this);
}

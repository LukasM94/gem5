#pragma once

#include <unordered_map>
#include <vector>

#include "base/statistics.hh"
#include "mem/port.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"

class SimpleCacheObjParams;

class SimpleCacheObj : public ClockedObject
{
  public:
    SimpleCacheObj(SimpleCacheObjParams* params);
    Port &getPort(const std::string &if_name, PortID idx=InvalidPortID)
                  override;

  private:
    struct SimpleCacheStats : public Stats::Group
    {
      SimpleCacheStats(Stats::Group* parent);
      Stats::Scalar    hits_;
      Stats::Scalar    misses_;
      Stats::Histogram miss_latency_;
      Stats::Formula   hitRatio_;
    };

    class AccessEvent : public Event
    {
      public:
        AccessEvent(SimpleCacheObj* cache, Packet* pkt) :
          Event(),
          cache_(cache),
          pkt_(pkt)
          {}
        void process() override
        {
          cache_->accessTiming(pkt_);
        }
      private:
        SimpleCacheObj* cache_;
        Packet*         pkt_;
    };

    class CPUSidePort : public ResponsePort
    {
      public:
        CPUSidePort(const std::string& name, unsigned int port_number,
                    SimpleCacheObj* owner) :
          ResponsePort(name, owner),
          owner_(owner),
          need_retry_(false),
          blocked_packet_(nullptr),
          port_number_(port_number)
        {}
        AddrRangeList getAddrRanges() const override;
        void sendPacket(Packet* pkt);
        void trySendRetry();
      protected:
        Tick recvAtomic(Packet* pkt) override;
        void recvFunctional(Packet* pkt) override;
        bool recvTimingReq(Packet* pkt) override;
        void recvRespRetry() override;
      private:
        SimpleCacheObj* owner_;
        bool need_retry_;
        Packet* blocked_packet_;
        unsigned int port_number_;
    };

    class MemSidePort : public RequestPort
    {
      public:
        MemSidePort(const std::string& name, SimpleCacheObj* owner) :
          RequestPort(name, owner),
          owner_(owner),
          blocked_packet_(nullptr)
        {}
        void sendPacket(Packet* pkt);
      protected:
        bool recvTimingResp(Packet* pkt) override;
        void recvReqRetry() override;
        void recvRangeChange() override;
      private:
        SimpleCacheObj* owner_;
        Packet* blocked_packet_;
    };

    bool handleRequest(Packet* pkt, int port_id);
    bool handleResponse(Packet* pkt);
    void sendResponse(Packet* pkt);
    void handleFunctional(Packet* pkt);
    void accessTiming(Packet* pkt);
    bool accessFunctional(Packet* pkt);
    void insert(Packet* pkt);
    AddrRangeList getAddrRanges() const;
    void sendRangeChange() const;

    // Latency to check the cache. Number of cycles for both hit and miss
    const Cycles latency_;
    // The block size for the cache
    const unsigned block_size_;
    // Number of blocks in the cache (size of cache / block size)
    const unsigned capacity_;
    // True if this cache is currently blocked waiting for a response.
    bool blocked_;
    // Packet that we are currently handling. Used for upgrading to larger
    // cache line sizes
    Packet* outstanding_packet_;
    // The port to send the response when we recieve it back
    int waiting_port_id_;
    // For tracking the miss latency
    Tick miss_time_;
    // An incredibly simple cache storage. Maps block addresses to data
    std::unordered_map<Addr, uint8_t*> storage_;

    struct SimpleCacheStats stats_;
    // Instantiation of the memory-side port
    MemSidePort mem_port_;
    // Instantiation of the CPU-side port
    std::vector<CPUSidePort> cpu_ports_;

    static const char NAME_CPU_PORTS[];
    static const char NAME_MEM_PORT[];
};
#pragma once

#include "sim/sim_object.hh"
#include "mem/port.hh"

class SimpleMemObjParams;

class SimpleMemObj : public SimObject
{
  public:
    SimpleMemObj(SimpleMemObjParams* params);
    Port& getPort(const std::string& if_name, PortID idx) override;

  private:

    class CPUSidePort : public ResponsePort
    {
      public:
        CPUSidePort(const std::string& name, SimpleMemObj* owner) :
          ResponsePort(name, owner), 
          owner_(owner),
          need_retry_(false),
          blocked_packet_(nullptr)
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
        SimpleMemObj* owner_;
        bool need_retry_;
        Packet* blocked_packet_;
    };

    class MemSidePort : public RequestPort
    {
      public:
        MemSidePort(const std::string& name, SimpleMemObj* owner) :
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
        SimpleMemObj* owner_;
        Packet* blocked_packet_;
    };

    void handleFunctional(Packet* pkt);
    AddrRangeList getAddrRanges() const;
    void sendRangeChange();
    bool handleRequest(Packet* pkt);
    bool handleResponse(Packet* pkt);

    bool blocked_;

    CPUSidePort inst_port_;
    CPUSidePort data_port_;

    MemSidePort mem_port_;

    static const char NAME_INST_PORT[];
    static const char NAME_DATA_PORT[];
    static const char NAME_MEM_PORT[];
};
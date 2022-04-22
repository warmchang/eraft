// MIT License

// Copyright (c) 2022 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <network/raft_server_transport.h>

namespace network
{

    RaftServerTransport::RaftServerTransport(std::shared_ptr<RaftClient> raftClient, std::shared_ptr<RaftRouter> raftRouter)
        : raftClient_(raftClient), raftRouter_(raftRouter) {}

    RaftServerTransport::~RaftServerTransport() {}

    void RaftServerTransport::Send(std::shared_ptr<raft_messagepb::RaftMessage> msg)
    {
        auto storeID = msg->to_peer().store_id();
        auto addr = msg->to_peer().add();
        SendStore(storeID, msg, addr);
    }

    void RaftServerTransport::SendStore(uint64_t storeId, std::shared_ptr<raft_messagepb::RaftMessage> msg, std::string addr)
    {
        WriteData(storeId, msg, addr);
    }

    void RaftServerTransport::WriteData(uint64_t storeId, std::string addr, std::shared_ptr<raft_messagepb::RaftMessage> msg)
    {
        raftClient_->Send(storeId, addr, *msg);
    }

} // namespace network

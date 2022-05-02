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

#ifndef ERAFT_PMEM_REDIS_H_
#define ERAFT_PMEM_REDIS_H_

#include <network/client.h>
#include <network/command.h>
#include <network/config.h>
#include <network/executor.h>
#include <network/raft_config.h>
#include <network/raft_stack.h>
#include <network/server.h>
#include <network/socket.h>
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

#define VERSION "1.0.0"

class PMemRedis : public Server {
 public:
  PMemRedis();
  ~PMemRedis();

  // bool ParseArgs(int ac, char* av[]);
  void InitSpdLogger();

  std::shared_ptr<network::RaftStack> GetRaftStack();

  static PMemRedis *GetInstance() {
    if (instance_ == nullptr) {
      instance_ = new PMemRedis();
      return instance_;
    }
    return instance_;
  }

 protected:
  static PMemRedis *instance_;

 private:
  std::shared_ptr<StreamSocket> _OnNewConnection(int fd, int tag) override;
  std::shared_ptr<network::RaftStack> raftStack_;

  bool _Init() override;
  bool _RunLogic() override;
  bool _Recycle() override;

  unsigned short port_;
};

#endif

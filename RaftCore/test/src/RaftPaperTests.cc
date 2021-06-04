#include <RaftCore/Raft.h>
#include <RaftCore/MemoryStorage.h>
#include <RaftCore/Util.h>
#include <gtest/gtest.h>

// TestFollowerUpdateTermFromMessage2AA、TestCandidateUpdateTermFromMessage2AA
// TestLeaderUpdateTermFromMessage2AA tests that if one server’s current term is
// smaller than the other’s, then it updates its current term to the larger
// value. If a candidate or leader discovers that its term is out of date,
// it immediately reverts to follower state.
// Reference: section 5.1
TEST(RaftPaperTests, TestFollowerUpdateTermFromMessage2AA) {
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::vector<uint64_t> peers = {1, 2, 3};
    eraft::Config c(1, peers, 10, 1, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
    r->BecomeFollower(1, 2);
    eraftpb::Message appendMsg;
    appendMsg.set_term(2);
    appendMsg.set_msg_type(eraftpb::MsgAppend);
    r->Step(appendMsg);
    ASSERT_EQ(r->term_, 2);
    ASSERT_EQ(r->state_, eraft::NodeState::StateFollower);
}

TEST(RaftPaperTests, TestCandidateUpdateTermFromMessage2AA) {
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::vector<uint64_t> peers = {1, 2, 3};
    eraft::Config c(1, peers, 10, 1, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
    r->BecomeCandidate();
    eraftpb::Message appendMsg;
    appendMsg.set_term(2);
    appendMsg.set_msg_type(eraftpb::MsgAppend);
    r->Step(appendMsg);
    ASSERT_EQ(r->term_, 2);
    ASSERT_EQ(r->state_, eraft::NodeState::StateFollower);
}

TEST(RaftPaperTests, TestLeaderUpdateTermFromMessage2AA) {
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::vector<uint64_t> peers = {1, 2, 3};
    eraft::Config c(1, peers, 10, 1, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
    r->BecomeCandidate();
    r->BecomeLeader();
    eraftpb::Message appendMsg;
    appendMsg.set_term(2);
    appendMsg.set_msg_type(eraftpb::MsgAppend);
    r->Step(appendMsg);
    ASSERT_EQ(r->term_, 2);
    ASSERT_EQ(r->state_, eraft::NodeState::StateFollower);
}

// TestStartAsFollower tests that when servers start up, they begin as followers.
// Reference: section 5.2
TEST(RaftPaperTests, TestStartAsFollower2AA) {
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::vector<uint64_t> peers = {1, 2, 3};
    eraft::Config c(1, peers, 10, 1, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
    ASSERT_EQ(r->state_, eraft::NodeState::StateFollower);
}

// TestLeaderBcastBeat tests that if the leader receives a heartbeat tick,
// it will send a MessageType_MsgHeartbeat with m.Index = 0, m.LogTerm=0 and empty entries
// as heartbeat to all followers.
// Reference: section 5.2
TEST(RaftPaperTests, TestLeaderBcastBeat2AA) {
    uint8_t hi = 1;
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::vector<uint64_t> peers = {1, 2, 3};
    eraft::Config c(1, peers, 10, hi, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
    r->BecomeCandidate();
    r->BecomeLeader();

    eraftpb::Message propMsg;
    propMsg.set_msg_type(eraftpb::MsgPropose);
    r->ReadMessage(); // clear message

    for(uint8_t i = 0; i < hi; i++) {
        r->Tick();
    }

    std::vector<eraftpb::Message> msgs = r->ReadMessage();
    for(auto msg : msgs) {
        std::cout << "msg.from(): " << msg.from() << " msg.to(): " << msg.to() << 
        " msg.term(): " << msg.term() << " msg type: " << eraft::MsgTypeToString(msg.msg_type()) << std::endl;
    }
}

// TestFollowerStartElection2AA tests that if a follower receives no communication
// over election timeout, it begins an election to choose a new leader. It
// increments its current term and transitions to candidate state. It then
// votes for itself and issues RequestVote RPCs in parallel to each of the
// other servers in the cluster.
// Reference: section 5.2
// Also if a candidate fails to obtain a majority, it will time out and
// start a new election by incrementing its term and initiating another
// round of RequestVote RPCs.
// Reference: section 5.2
TEST(RaftPaperTests, TestFollowerStartElection2AA) {
    // election timeout
    uint8_t et = 10;
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::vector<uint64_t> peers = {1, 2, 3};
    eraft::Config c(1, peers, et, 1, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
    r->BecomeFollower(1, 2);

    for(uint8_t i = 1; i < 2*et; i++) {
        r->Tick();
    }

    ASSERT_EQ(r->term_, 2);

    ASSERT_EQ(r->state_, eraft::NodeState::StateCandidate);

    ASSERT_TRUE(r->votes_[r->id_]);

    std::vector<eraftpb::Message> msgs = r->ReadMessage();
    
    for(auto msg : msgs) {
        std::cout << "msg.from(): " << msg.from() << " msg.to(): " << msg.to() << 
        " msg.term(): " << msg.term() << " msg type: " << eraft::MsgTypeToString(msg.msg_type()) << std::endl;
    }
}

TEST(RaftPaperTests, TestCandidateStartNewElection2AA) {
    // election timeout
    uint8_t et = 10;
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::vector<uint64_t> peers = {1, 2, 3};
    eraft::Config c(1, peers, et, 3, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
    r->BecomeCandidate();

    for(uint8_t i = 1; i < 2*et; i++) {
        r->Tick();
    }

    ASSERT_EQ(r->term_, 2);

    ASSERT_EQ(r->state_, eraft::NodeState::StateCandidate);

    ASSERT_TRUE(r->votes_[r->id_]);

    std::vector<eraftpb::Message> msgs = r->ReadMessage();
    
    for(auto msg : msgs) {
        std::cout << "msg.from(): " << msg.from() << " msg.to(): " << msg.to() << 
        " msg.term(): " << msg.term() << " msg type: " << eraft::MsgTypeToString(msg.msg_type()) << std::endl;
    }
}

// TestLeaderElectionInOneRoundRPC tests all cases that may happen in
// leader election during one round of RequestVote RPC:
// a) it wins the election
// b) it loses the election
// c) it is unclear about the result
// Reference: section 5.2

struct TestEntry
{
    TestEntry(uint64_t size, std::map<uint64_t, bool> votes, eraft::NodeState state) {
        this->size_ = size;
        this->votes_ = votes;
        this->state_ = state;
    }
    uint64_t size_;
    std::map<uint64_t, bool> votes_;
    eraft::NodeState state_;
};

std::vector<uint64_t> IdsBySize(uint64_t size) {
    std::vector<uint64_t> ids;
    for(uint64_t i = 0; i < size; i++) {
        ids.push_back(1 + i);
    }
    return ids;
}

TEST(RaftPaperTests, TestLeaderElectionInOneRoundRPC2AA) {
    std::vector<TestEntry> tests;
    
    // win the election when receiving votes from a majority of the servers
    tests.push_back(TestEntry(1, std::map<uint64_t, bool>{}, eraft::NodeState::StateLeader));
    tests.push_back(TestEntry(3, std::map<uint64_t, bool>{ {2, true}, {3, true} }, eraft::NodeState::StateLeader));
    tests.push_back(TestEntry(3, std::map<uint64_t, bool>{ {2, true} }, eraft::NodeState::StateLeader));
    tests.push_back(TestEntry(5, std::map<uint64_t, bool>{ {2, true}, {3, true}, {4, true}, {5, true} }, eraft::NodeState::StateLeader));
    tests.push_back(TestEntry(5, std::map<uint64_t, bool>{ {2, true}, {3, true}, {4, true} }, eraft::NodeState::StateLeader));
    tests.push_back(TestEntry(5, std::map<uint64_t, bool>{ {2, true}, {3, true} }, eraft::NodeState::StateLeader));

    tests.push_back(TestEntry(3, std::map<uint64_t, bool>{}, eraft::NodeState::StateCandidate));
    tests.push_back(TestEntry(5, std::map<uint64_t, bool>{ {2, true} }, eraft::NodeState::StateCandidate));
    tests.push_back(TestEntry(5, std::map<uint64_t, bool>{ {2, false}, {3, false} }, eraft::NodeState::StateCandidate));
    tests.push_back(TestEntry(5, std::map<uint64_t, bool>{}, eraft::NodeState::StateCandidate));

    for(auto tt : tests) {
        std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
        std::vector<uint64_t> peers = {};
        eraft::Config c(1, IdsBySize(tt.size_), 10, 1, memSt);
        std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
        eraftpb::Message hupMsg;
        hupMsg.set_from(1);
        hupMsg.set_to(1);
        hupMsg.set_msg_type(eraftpb::MsgHup);
        r->Step(hupMsg);
        uint8_t i = 0;
        for(auto vote: tt.votes_) {
            eraftpb::Message rvMsg;
            rvMsg.set_from(vote.first);
            rvMsg.set_to(1);
            rvMsg.set_term(r->term_);
            rvMsg.set_msg_type(eraftpb::MsgRequestVoteResponse);
            rvMsg.set_reject(!vote.second);
            r->Step(rvMsg);
        }
        // std::cout << " r->state_: " << eraft::StateToString(r->state_) << " r->term_:" << r->term_ << std::endl;
        ASSERT_EQ(eraft::StateToString(r->state_), eraft::StateToString(tt.state_));
        ASSERT_EQ(r->term_, 1);
    }
}


struct TestEntry1
{
    TestEntry1(uint64_t vote, uint64_t nvote, uint64_t wreject) {
        this->vote_ = vote;
        this->nvote_ = nvote;
        this->wreject_ = wreject;
    }
    uint64_t vote_;
    uint64_t nvote_;
    bool wreject_;
};

// TestFollowerVote tests that each follower will vote for at most one
// candidate in a given term, on a first-come-first-served basis.
// Reference: section 5.2
TEST(RaftPaperTests, TestFollowerVote2AA) {
    std::vector<TestEntry1> tests;
    tests.push_back(TestEntry1(eraft::NONE, 1, false));
    tests.push_back(TestEntry1(eraft::NONE, 2, false));
    tests.push_back(TestEntry1(1, 1, false));
    tests.push_back(TestEntry1(2, 2, false));
    tests.push_back(TestEntry1(1, 2, true));
    tests.push_back(TestEntry1(2, 1, true));

    for(auto tt : tests) {
        std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
        std::vector<uint64_t> peers = {1, 2, 3};
        eraft::Config c(1, peers, 10, 1, memSt);
        std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
        r->term_ = 1;
        r->vote_ = tt.vote_;

        eraftpb::Message msg;
        msg.set_from(tt.nvote_);
        msg.set_to(1);
        msg.set_term(1);
        msg.set_msg_type(eraftpb::MsgRequestVote);

        r->Step(msg);

        std::vector<eraftpb::Message> msgs = r->ReadMessage();
        // 	{From: 1, To: tt.nvote, Term: 1, MsgType: pb.MessageType_MsgRequestVoteResponse, Reject: tt.wreject}
        for(auto msg : msgs) {
        std::cout << "msg.from(): " << msg.from() << " msg.to(): " << msg.to() << 
        " msg.term(): " << msg.term() << " msg type: " << eraft::MsgTypeToString(msg.msg_type()) << "msg.reject(): " << std::boolalpha << msg.reject() << std::endl;
        }
    }
}

// TestCandidateFallback tests that while waiting for votes,
// if a candidate receives an AppendEntries RPC from another server claiming
// to be leader whose term is at least as large as the candidate's current term,
// it recognizes the leader as legitimate and returns to follower state.
// Reference: section 5.2
TEST(RaftPaperTests, TestCandidateFallback2AA) {
    eraftpb::Message msg1;
    msg1.set_from(2);
    msg1.set_to(1);
    msg1.set_term(1);
    msg1.set_msg_type(eraftpb::MsgAppend);
    eraftpb::Message msg2;
    msg2.set_from(2);
    msg2.set_to(1);
    msg2.set_term(2);
    msg2.set_msg_type(eraftpb::MsgAppend);
    std::vector<eraftpb::Message> tests;
    tests.push_back(msg1);
    tests.push_back(msg2);
    for(auto tt : tests) {
        std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
        std::vector<uint64_t> peers = {1, 2, 3};
        eraft::Config c(1, peers, 10, 1, memSt);
        std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
        eraftpb::Message hupMsg;
        hupMsg.set_from(1);
        hupMsg.set_to(1);
        hupMsg.set_msg_type(eraftpb::MsgHup);
        r->Step(hupMsg);
        ASSERT_EQ(eraft::StateToString(r->state_), eraft::StateToString(eraft::NodeState::StateCandidate));

        r->Step(tt);

        ASSERT_EQ(eraft::StateToString(r->state_), eraft::StateToString(eraft::NodeState::StateFollower));
        ASSERT_EQ(r->term_, tt.term());
    }
}

// TestFollowerElectionTimeoutRandomized2AA、TestCandidateElectionTimeoutRandomized2AA tests that election timeout for
// follower or candidate is randomized.
// Reference: section 5.2
TEST(RaftPaperTests, TestFollowerElectionTimeoutRandomized2AA) {
    uint8_t et = 10;
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::vector<uint64_t> peers = {1, 2, 3};
    eraft::Config c(1, peers, et, 1, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
    std::map<uint64_t, bool> timeouts;
    for(uint64_t round = 0; round < 50*et; round++) {
        r->BecomeFollower(r->term_+1, 2);

        uint64_t time = 0;
        while (r->ReadMessage().size() == 0)
        {
            r->Tick();
            time++;
        }
        timeouts[time] = true;
    }

    for(uint64_t d = et + 1; d < 2*et; d++) {
        if(!timeouts[d]) {
            std::cerr << "timeout in " << d << " ticks should happen" << std::endl;
        }
    }
}

TEST(RaftPaperTests, TestCandidateElectionTimeoutRandomized2AA) {
    uint8_t et = 10;
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::vector<uint64_t> peers = {1, 2, 3};
    eraft::Config c(1, peers, et, 1, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
    std::map<uint64_t, bool> timeouts;
    for(uint64_t round = 0; round < 50*et; round++) {
        r->BecomeCandidate();

        uint64_t time = 0;
        while (r->ReadMessage().size() == 0)
        {
            r->Tick();
            time++;
        }
        timeouts[time] = true;
    }

    for(uint64_t d = et + 1; d < 2*et; d++) {
        if(!timeouts[d]) {
            std::cerr << "timeout in " << d << " ticks should happen" << std::endl;
        }
    }
}


// TestFollowersElectionTimeoutNonconflict2AA tests that in most cases only a
// single server(follower or candidate) will time out, which reduces the
// likelihood of split vote in the new election.
// Reference: section 5.2

TEST(RaftPaperTests, TestFollowersElectionTimeoutNonconflict2AA) {
    uint64_t et = 10;
    uint64_t size = 5;
    std::vector<uint64_t> ids = IdsBySize(size);

    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    eraft::Config c(1, ids, et, 1, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);

    std::shared_ptr<eraft::StorageInterface> memSt1 = std::make_shared<eraft::MemoryStorage>();
    eraft::Config c1(2, ids, et, 1, memSt1);
    std::shared_ptr<eraft::RaftContext> r1 = std::make_shared<eraft::RaftContext>(c1);

    std::shared_ptr<eraft::StorageInterface> memSt2 = std::make_shared<eraft::MemoryStorage>();
    eraft::Config c2(3, ids, et, 1, memSt2);
    std::shared_ptr<eraft::RaftContext> r2 = std::make_shared<eraft::RaftContext>(c2);

    std::shared_ptr<eraft::StorageInterface> memSt3 = std::make_shared<eraft::MemoryStorage>();
    eraft::Config c3(4, ids, et, 1, memSt3);
    std::shared_ptr<eraft::RaftContext> r3 = std::make_shared<eraft::RaftContext>(c3);

    std::shared_ptr<eraft::StorageInterface> memSt4 = std::make_shared<eraft::MemoryStorage>();
    eraft::Config c4(5, ids, et, 1, memSt4);
    std::shared_ptr<eraft::RaftContext> r4 = std::make_shared<eraft::RaftContext>(c4);

    uint64_t conflicts = 0;
    for(uint64_t round = 0; round < 1000; round++) {
        r->BecomeFollower(r->term_+1, eraft::NONE);
        uint64_t timeoutNum = 0;
        while (timeoutNum == 0)
        {
            r->Tick();
            if(r->ReadMessage().size() > 0) {  // state machine timeout
                timeoutNum++;
            }
            r1->Tick();
            if(r1->ReadMessage().size() > 0) {
                timeoutNum++;
            }
            r2->Tick();
            if(r2->ReadMessage().size() > 0) {
                timeoutNum++;
            }
            r3->Tick();
            if(r3->ReadMessage().size() > 0) {
                timeoutNum++;
            }
            r4->Tick();
            if(r4->ReadMessage().size() > 0) {
                timeoutNum++;
            }
        }
        if(timeoutNum > 1) {
            conflicts++;
        }
    }
    double probabilityOfConflicts = double(conflicts)/1000.0;
    std::cout <<  "probability of conflicts = " << probabilityOfConflicts << std::endl;
    if(probabilityOfConflicts > 0.3) {
        std::cerr << "probability of conflicts = " << probabilityOfConflicts << " want <= 0.3" << std::endl;
    } 
}

TEST(RaftPaperTests, TestCandidatesElectionTimeoutNonconflict2AA) {
    uint64_t et = 10;
    uint64_t size = 5;
    std::vector<uint64_t> ids = IdsBySize(size);

    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    eraft::Config c(1, ids, et, 1, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);

    std::shared_ptr<eraft::StorageInterface> memSt1 = std::make_shared<eraft::MemoryStorage>();
    eraft::Config c1(2, ids, et, 1, memSt1);
    std::shared_ptr<eraft::RaftContext> r1 = std::make_shared<eraft::RaftContext>(c1);

    std::shared_ptr<eraft::StorageInterface> memSt2 = std::make_shared<eraft::MemoryStorage>();
    eraft::Config c2(3, ids, et, 1, memSt2);
    std::shared_ptr<eraft::RaftContext> r2 = std::make_shared<eraft::RaftContext>(c2);

    std::shared_ptr<eraft::StorageInterface> memSt3 = std::make_shared<eraft::MemoryStorage>();
    eraft::Config c3(4, ids, et, 1, memSt3);
    std::shared_ptr<eraft::RaftContext> r3 = std::make_shared<eraft::RaftContext>(c3);

    std::shared_ptr<eraft::StorageInterface> memSt4 = std::make_shared<eraft::MemoryStorage>();
    eraft::Config c4(5, ids, et, 1, memSt4);
    std::shared_ptr<eraft::RaftContext> r4 = std::make_shared<eraft::RaftContext>(c4);

    uint64_t conflicts = 0;
    for(uint64_t round = 0; round < 1000; round++) {
        r->BecomeCandidate();
        uint64_t timeoutNum = 0;
        while (timeoutNum == 0)
        {
            r->Tick();
            if(r->ReadMessage().size() > 0) {  // state machine timeout
                timeoutNum++;
            }
            r1->Tick();
            if(r1->ReadMessage().size() > 0) {
                timeoutNum++;
            }
            r2->Tick();
            if(r2->ReadMessage().size() > 0) {
                timeoutNum++;
            }
            r3->Tick();
            if(r3->ReadMessage().size() > 0) {
                timeoutNum++;
            }
            r4->Tick();
            if(r4->ReadMessage().size() > 0) {
                timeoutNum++;
            }
        }
        if(timeoutNum > 1) {
            conflicts++;
        }
    }
    double probabilityOfConflicts = double(conflicts)/1000.0;
    std::cout <<  "probability of conflicts = " << probabilityOfConflicts << std::endl;
    if(probabilityOfConflicts > 0.3) {
        std::cerr << "probability of conflicts = " << probabilityOfConflicts << " want <= 0.3" << std::endl;
    } 
}


static eraftpb::Message AcceptAndReply(eraftpb::Message m) {
    if(m.msg_type() != eraftpb::MsgAppend) {
        exit(-1);
    }
    eraftpb::Message reply;
    reply.set_from(m.to());
    reply.set_to(m.from());
    reply.set_term(m.term());
    reply.set_msg_type(eraftpb::MsgAppendResponse);
    reply.set_index(m.index() + m.entries().size());
    return reply;
}

static bool CommitNoopEntry(std::shared_ptr<eraft::RaftContext> r, std::shared_ptr<eraft::StorageInterface> s) {
    if(r->state_ != eraft::NodeState::StateLeader) {
        return false;
    }
    for(auto pr : r->prs_) {
        if(pr.first == r->id_) {
            continue;
        }
        r->SendAppend(pr.first);
    }
    // simulate the response of MessageType_MsgAppend
    std::vector<eraftpb::Message> msgs = r->ReadMessage();
    for(auto m : msgs) {
        if(m.msg_type() != eraftpb::MsgAppend || m.entries().size() != 1 || !m.entries()[0].data().empty()) {
            return false;
        }
        // std::cout << "AcceptAndReply " << "m.from(): " << m.from() << "m.to() " << m.to() << std::endl;
        r->Step(AcceptAndReply(m));
    }
    // ignore further messages to refresh followers' commit index.
    r->ReadMessage();
    s->Append(r->raftLog_->UnstableEntries());
    r->raftLog_->applied_ = r->raftLog_->commited_;
    r->raftLog_->stabled_ = r->raftLog_->LastIndex();
    return true;
}

// TestLeaderStartReplication tests that when receiving client proposals,
// the leader appends the proposal to its log as a new entry, then issues
// AppendEntries RPCs in parallel to each of the other servers to replicate
// the entry. Also, when sending an AppendEntries RPC, the leader includes
// the index and term of the entry in its log that immediately precedes
// the new entries.
// Also, it writes the new entry into stable storage.
// Reference: section 5.3

TEST(RaftPaperTests, TestLeaderStartReplication2AB) {
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::vector<uint64_t> ids = IdsBySize(3);
    eraft::Config c(1, ids, 10, 1, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);

    r->BecomeCandidate();
    r->BecomeLeader();
    ASSERT_TRUE(CommitNoopEntry(r, memSt));
    uint64_t li = r->raftLog_->LastIndex();

    eraftpb::Message proposeMsg;
    proposeMsg.set_from(1);
    proposeMsg.set_to(1);
    proposeMsg.set_msg_type(eraftpb::MsgPropose);
    eraftpb::Entry* eptr = proposeMsg.add_entries();
    eptr->set_data("some data");

    r->Step(proposeMsg);

    ASSERT_EQ(r->raftLog_->LastIndex(), li + 1);
    ASSERT_EQ(r->raftLog_->commited_, li);

    std::vector<eraftpb::Message> msgs = r->ReadMessage();
    
    std::cout << "====================r->ReadMessage()====================" << std::endl;
    for(auto m : msgs) {
        std::cout << eraft::MessageToString(m) << std::endl;
    }

    std::cout << "=============r->raftLog_->UnstableEntries()=============" << std::endl;
    // wents := []pb.Entry{{Index: li + 1, Term: 1, Data: []byte("some data")}}
    std::cout << "stabled_: " << r->raftLog_->stabled_ << "commited_: " << r->raftLog_->commited_ << std::endl;
    std::vector<eraftpb::Entry> ents = r->raftLog_->UnstableEntries();
    for(auto e : ents) {
        std::cout << eraft::EntryToString(e) << std::endl;
    }
}

// TestLeaderCommitEntry tests that when the entry has been safely replicated,
// the leader gives out the applied entries, which can be applied to its state
// machine.
// Also, the leader keeps track of the highest index it knows to be committed,
// and it includes that index in future AppendEntries RPCs so that the other
// servers eventually find out.
// Reference: section 5.3

TEST(RaftPaperTests, TestLeaderCommitEntry2AB) {
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::vector<uint64_t> ids = IdsBySize(3);
    eraft::Config c(1, ids, 10, 1, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
    r->BecomeCandidate();
    r->BecomeLeader();
    CommitNoopEntry(r, memSt);
    uint64_t li = r->raftLog_->LastIndex();
    eraftpb::Message proposeMsg;
    proposeMsg.set_from(1);
    proposeMsg.set_to(1);
    proposeMsg.set_msg_type(eraftpb::MsgPropose);
    eraftpb::Entry* eptr = proposeMsg.add_entries();
    eptr->set_data("some data");
    r->Step(proposeMsg);

    std::vector<eraftpb::Message> msgs = r->ReadMessage();
    std::cout << "====================r->ReadMessage()====================" << std::endl;
    for(auto m : msgs) {
        r->Step(AcceptAndReply(m));
    }

    ASSERT_EQ(r->raftLog_->commited_, li + 1);

    // wents := []pb.Entry{{Index: li + 1, Term: 1, Data: []byte("some data")}}
    std::vector<eraftpb::Entry> ents = r->raftLog_->NextEnts();
    for(auto e : ents) {
        std::cout << eraft::EntryToString(e) << std::endl;
    }

    std::vector<eraftpb::Message> msgsAppend = r->ReadMessage();
    uint8_t i = 0;
    for(auto m : msgsAppend) {
        std::cout << eraft::MessageToString(m) << std::endl;
    }
}


struct TestEntry2
{
    TestEntry2(uint64_t size, std::map<uint64_t, bool> acceptors, bool wack) {
        this->size_ = size;
        this->acceptors_ = acceptors;
        this->wack_ = wack;
    }

    uint64_t size_;
    std::map<uint64_t, bool> acceptors_;
    bool wack_;
};

// TestLeaderAcknowledgeCommit tests that a log entry is committed once the
// leader that created the entry has replicated it on a majority of the servers.
// Reference: section 5.3
TEST(RaftPaperTests, TestLeaderAcknowledgeCommit2AB) {

    auto runtest = [](std::vector<TestEntry2> cases) {
        for(auto iter = cases.begin(); iter != cases.end(); iter++) {
            std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
            std::vector<uint64_t> ids = IdsBySize(iter->size_);
            eraft::Config c(1, ids, 10, 1, memSt);
            std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
            r->BecomeCandidate();
            r->BecomeLeader();
            CommitNoopEntry(r, memSt);
            uint64_t li = r->raftLog_->LastIndex();
            eraftpb::Message proposeMsg;
            proposeMsg.set_from(1);
            proposeMsg.set_to(1);
            proposeMsg.set_msg_type(eraftpb::MsgPropose);
            eraftpb::Entry* eptr = proposeMsg.add_entries();
            
            eptr->set_data("some data");
            r->Step(proposeMsg);

            std::vector<eraftpb::Message> msgs = r->ReadMessage();

            for(auto m : msgs) {
                if(iter->acceptors_[m.to()]) {
                    std::cout << "AcceptAndReply" << std::endl;
                    r->Step(AcceptAndReply(m));
                }
            }

            std::cout << "r->raftLog_->commited_: " << r->raftLog_->commited_ << " li:" << li << std::endl;
            ASSERT_EQ(r->raftLog_->commited_ > li, iter->wack_);
        }
    };

    std::vector<TestEntry2> testCases { 
        TestEntry2(3, std::map<uint64_t, bool>{ {2, true} }, true),
        TestEntry2(3, std::map<uint64_t, bool>{ {2, true}, {3, true} }, true),
        TestEntry2(5, std::map<uint64_t, bool>{ {2, true} }, false),
        TestEntry2(5, std::map<uint64_t, bool>{ {2, true}, {3, true} }, true),
        TestEntry2(5, std::map<uint64_t, bool>{ {2, true}, {3, true}, {4, true} }, true),
        TestEntry2(5, std::map<uint64_t, bool>{ {2, true}, {3, true}, {4, true}, {5, true} }, true),
    };

    runtest(testCases);

    std::vector<TestEntry2> testCasesEx { 
        TestEntry2(1, {}, true), 
        TestEntry2(3, {}, false),
        TestEntry2(5, {}, false),
    };

    runtest(testCasesEx);
}


// TestLeaderCommitPrecedingEntries tests that when leader commits a log entry,
// it also commits all preceding entries in the leader’s log, including
// entries created by previous leaders.
// Also, it applies the entry to its local state machine (in log order).
// Reference: section 5.3
TEST(RaftPaperTests, TestLeaderCommitPrecedingEntries2AB) {
    eraftpb::Entry en1, en2, en3;
    en1.set_term(2);
    en1.set_index(1);

    en2.set_term(1);
    en2.set_index(1);

    en3.set_term(2);
    en3.set_index(2);

    std::vector<eraftpb::Entry> ens1  = { };
    std::vector<eraftpb::Entry> ens2  = { en1 };
    std::vector<eraftpb::Entry> ens3  = { en2, en3 };
    std::vector<eraftpb::Entry> ens4  = { en2 };

    std::vector< std::vector<eraftpb::Entry> > testCases = { ens1, ens2, ens3, ens4 };
    // std::vector< std::vector<eraftpb::Entry> > testCases = { ens4 };

    for(auto tt: testCases) {
        std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
        memSt->Append(tt);

        std::vector<uint64_t> ids = IdsBySize(3);
        eraft::Config c(1, ids, 10, 1, memSt);
        std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
        r->term_ = 2;
        r->BecomeCandidate();
        r->BecomeLeader();

        eraftpb::Message proposeMsg;
        proposeMsg.set_from(1);
        proposeMsg.set_to(1);
        proposeMsg.set_msg_type(eraftpb::MsgPropose);
        eraftpb::Entry* eptr = proposeMsg.add_entries();
        
        eptr->set_data("some data");
        r->Step(proposeMsg);

        std::vector<eraftpb::Message> msgs = r->ReadMessage();
        for(auto m : msgs) {
            r->Step(AcceptAndReply(m));
        }

        uint64_t li = tt.size();
        // std::cout << "tt.size() " << li << " applied_: " << r->raftLog_->applied_ << " commited_: " << r->raftLog_->commited_ << " firstIndex_: " << r->raftLog_->firstIndex_ << std::endl;
        std::vector<eraftpb::Entry> ents = r->raftLog_->NextEnts();
        uint8_t i = 1;
        std::cout << "ROUND: " << i << std::endl;
        for(auto e : ents) {
            // wents := append(tt, pb.Entry{Term: 3, Index: li + 1}, pb.Entry{Term: 3, Index: li + 2, Data: []byte("some data")})
            std::cout << eraft::EntryToString(e) << std::endl;
            i++;
        }
    }
}


// TestFollowerCommitEntry tests that once a follower learns that a log entry
// is committed, it applies the entry to its local state machine (in log order).
// Reference: section 5.3
TEST(RaftPaperTests, TestFollowerCommitEntry2AB) {
    eraftpb::Entry en1, en2, en3, en4;
    en1.set_term(1); en1.set_index(1);  en1.set_data("some data");
    en2.set_term(1); en2.set_index(2);  en2.set_data("some data2");
    en3.set_term(1); en3.set_index(1);  en3.set_data("some data2");
    en4.set_term(1); en4.set_index(2); en4.set_data("some data");

    std::vector<eraftpb::Entry* > ens1, ens2, ens3, ens4;
    ens1.push_back(&en1);
    ens2.push_back(&en1);
    ens2.push_back(&en2);
    ens3.push_back(&en3);
    ens3.push_back(&en4);
    ens4.push_back(&en1);
    ens4.push_back(&en2);

    std::vector<std::pair<std::vector<eraftpb::Entry* >, uint64_t> > tests = { {ens1, 1}, {ens2, 2}, {ens3, 2}, {ens4, 1} };
    for(auto tt : tests) {
        std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
        std::vector<uint64_t> ids = IdsBySize(3);
        eraft::Config c(1, ids, 10, 1, memSt);
        std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);

        r->BecomeFollower(1, 2);

        eraftpb::Message appEnd;
        appEnd.set_from(2);
        appEnd.set_to(1);
        appEnd.set_msg_type(eraftpb::MsgAppend);
        appEnd.set_term(1);
        for(auto e : tt.first) {
            eraftpb::Entry *en = appEnd.add_entries();
            en->set_index(e->index());
            en->set_data(e->data());
            en->set_term(e->term());
            en->set_entry_type(e->entry_type());
        }
        appEnd.set_commit(tt.second);

        r->Step(appEnd);

        std::cout << r->raftLog_->commited_ << std::endl;
        ASSERT_EQ(r->raftLog_->commited_, tt.second);

        std::vector<eraftpb::Entry> ents = r->raftLog_->NextEnts();
        std::cout << "ROUND START " << std::endl;
        for(auto e : ents) {
            std::cout << eraft::EntryToString(e) << std::endl;
        }
        std::cout << "ROUND END " << std::endl;
    }
}


struct TestEntry3
{
    TestEntry3(uint64_t term, uint64_t index, bool wreject) {
        this->term_ = term;
        this->index_ = index;
        this->wreject_ = wreject;
    }

    uint64_t term_;

    uint64_t index_;

    bool wreject_;
};

// TestFollowerCheckMessageType_MsgAppend tests that if the follower does not find an
// entry in its log with the same index and term as the one in AppendEntries RPC,
// then it refuses the new entries. Otherwise it replies that it accepts the
// append entries.
// Reference: section 5.3
TEST(RaftPaperTests, TestFollowerCheckMessageType_MsgAppend2AB) {
    eraftpb::Entry en1, en2;
    en1.set_term(1); en1.set_index(1);
    en2.set_term(2); en2.set_index(2);

    std::vector<TestEntry3> tests = { 
        // match with committed entries
        // TestEntry3(0 , 0, false),
        TestEntry3(1, 1, false),

        // // match with uncommited entries
        // TestEntry3(en2.term(), en2.index(), false),

        // // unmatch with existing entry
        // TestEntry3(en1.term(), en2.index(), true),

        // // unexisting entry
        // TestEntry3(en2.term() + 1, en2.index() + 1, true),

    };

    for(auto tt : tests) {
        std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
        std::vector<uint64_t> ids = IdsBySize(3);
        eraft::Config c(1, ids, 10, 1, memSt);
        std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
        r->raftLog_->commited_ = 1;
        r->BecomeFollower(2, 2);
        r->ReadMessage(); // clear message

        eraftpb::Message appEnd;
        appEnd.set_from(2);
        appEnd.set_to(1);
        appEnd.set_msg_type(eraftpb::MsgAppend);
        appEnd.set_term(2);
        appEnd.set_log_term(tt.term_);
        appEnd.set_index(tt.index_);

        r->Step(appEnd);

        std::vector<eraftpb::Message> msgs = r->ReadMessage();
        ASSERT_EQ(msgs.size(), 1);
        ASSERT_EQ(msgs[0].term(), 2);
        // ASSERT_EQ(msgs[0].reject(), tt.wreject_);
        // std::cout << "ROUND START " << std::endl;
        for(auto m : msgs) {
            // std::cout << eraft::MessageToString(m) << std::endl;
        }
        // std::cout << "ROUND END " << std::endl;

    }


}

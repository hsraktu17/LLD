#include<bits/stdc++.h>

using namespace std;

struct Poll{
    string PollId;
    string question;
    vector<string> options;
    unordered_map<string , int> votes; //option, votes
    time_t createdAt;
};

class PollManager{
private:
    unordered_map<string , Poll> polls; //pollid, poll
    mutex mtx;
    int pollcount = 0;

public:
    string createPoll(const string &question, const vector<string> &options){
        lock_guard<mutex> lock(mtx);
        string  pollId = to_string(++pollcount);
        Poll poll = {pollId, question, options, {}, time(0)};
        
        for(auto &option: options){
            poll.votes[option] = 0;
        }
        polls[pollId] = poll;
        return pollId;
    }

    bool updatePoll(const string &pollId, const string &question, const vector<string> &options){
        lock_guard<mutex> lock(mtx);
        if(polls.find(pollId) == polls.end()) return false;
        polls[pollId].question = question;
        polls[pollId].options = options;
        for(auto &option: options){
            polls[pollId].votes[option] = 0;
        }
        return true;
    }

    bool deletePost(const string &pollId) {
        lock_guard<mutex> lock(mtx);
        return polls.erase(pollId) > 0;
    }

    Poll &getPoll(const string &pollId){
        return polls[pollId];
    }

    bool pollExist(const string &pollId){
        return polls.find(pollId) != polls.end();
    }
};

class VoteManager {
private:
    unordered_map<string, unordered_map<string, string>> votes;
    mutex mtx;
    PollManager &pollManager;
public:
    VoteManager(PollManager &pm) : pollManager(pm) {}
        
    bool voteInPoll(const string &pollId, const string &userId, const string &option) {
        lock_guard<mutex> lock(mtx);
        if (!pollManager.pollExist(pollId)) return false;
        if (votes[pollId].find(userId) != votes[pollId].end()) return false; 
            
        Poll& poll = pollManager.getPoll(pollId);
        if (poll.votes.find(option) == poll.votes.end()) return false;
            
        votes[pollId][userId] = option;
        poll.votes[option]++;
        return true;
    }
        
    unordered_map<string, int> viewPollResults(const string &pollId) {
        if (!pollManager.pollExist(pollId)) return {};
        return pollManager.getPoll(pollId).votes;
    }
};
int main(){
    return 0;
}

#include<bits/stdc++.h>
#include<mutex>

using namespace std;

struct Poll{
    string PollID;
    string questions;
    vector<string> options;
    unordered_map<string, int> votes;
    time_t createdAt;
};

class PollManager{
private:
    unordered_map<string, Poll> polls;
    mutex mtx;
    int pollcount = 0;

public:
    string createPoll(const string &question, vector<string> options){
        lock_guard<mutex>lock(mtx);
        string pollId = to_string(++pollcount);
        Poll poll = {pollId, question, options, {}, time(0)};
        for(const auto &option: options){
            poll.votes[option] = 0;
        }
        polls[pollId] = poll;
        return pollId;
    }

    bool updatePoll(const string pollId,const string &question, vector<string> options){
        lock_guard<mutex>lock(mtx);
        if(polls.find(pollId) == polls.end()) return false;
        polls[pollId].questions = question; 
        polls[pollId].options = options;
        polls[pollId].votes.clear();
        for(const auto &option: options){
            polls[pollId].votes[option] = 0;
        }
        return true;
    }

    bool deletePoll(const string pollId){
        lock_guard<mutex>lock(mtx);
        return polls.erase(pollId) > 0;
    }

    Poll &getPoll(const string &pollId){
        lock_guard<mutex> lock(mtx);
        return polls[pollId];
    }

    bool pollExist(const string &pollId){
        lock_guard<mutex> lock(mtx) ;
        return polls.find(pollId) != polls.end();
    }
};

class VoteManager{
private:
    unordered_map<string,unordered_map<string,string>> votes;
    mutex mtx;
    PollManager &pollManager;
public:
    VoteManager(PollManager &pm) : pollManager(pm) {}

    bool voteInPoll(const string &pollId, const string &userId, const string &options){
        lock_guard<mutex> lock(mtx);
        if(!pollManager.pollExist(pollId)) return false;
        if(votes[pollId].find(userId) != votes[pollId].end()){
            return false;
        }
        
        Poll &poll = pollManager.getPoll(pollId);
        if (poll.votes.find(options) == poll.votes.end()) {
            return false; 
        }
        votes[pollId][userId] = options;
        poll.votes[options]++;
        return true;
    }

    unordered_map<string, int> viewPollResult(const string& pollId){
        if(!pollManager.pollExist(pollId)){
            return {};
        }
        return pollManager.getPoll(pollId).votes;
    }
};

int main(){

    PollManager pollManager;
    VoteManager voteManager(pollManager);
    
    cout << "Creating Poll..." << endl;
    string pollId = pollManager.createPoll("What is your favorite color?", {"Red", "Blue", "Green", "Yellow"});
    cout << "Poll Created with ID: " << pollId << endl;
    Poll poll = pollManager.getPoll(pollId);
    cout << "Question: " << poll.questions << endl;
    cout << "Options:" << endl;
    for (const auto &option : poll.options) {
        cout << "- " << option << endl;
    }
    
    cout << "Updating Poll..." << endl;
    if (pollManager.updatePoll(pollId, "What is your favorite season?", {"Spring", "Summer", "Autumn", "Winter"})) {
        cout << "Poll Updated Successfully." << endl;
        poll = pollManager.getPoll(pollId);
        cout << "Updated Question: " << poll.questions << endl;
        cout << "Updated Options:" << endl;
        for (const auto &option : poll.options) {
            cout << "- " << option << endl;
        }
    }
    
    cout << "Voting in Poll..." << endl;
    if (voteManager.voteInPoll(pollId, "user1", "Spring")) {
        cout << "User1 voted for Spring." << endl;
    }
    if (!voteManager.voteInPoll(pollId, "user1", "Winter")) {
        cout << "User1 cannot vote twice." << endl;
    }
    if (voteManager.voteInPoll(pollId, "user2", "Winter")) {
        cout << "User2 voted for Winter." << endl;
    }
    
    cout << "Viewing Poll Results..." << endl;
    unordered_map<string, int> results = voteManager.viewPollResult(pollId);
    for (const auto &[option, count] : results) {
        cout << option << ": " << count << " votes" << endl;
    }
    
    cout << "Deleting Poll..." << endl;
    if (pollManager.deletePoll(pollId)) {
        cout << "Poll Deleted Successfully." << endl;
    }
    
    
    return 0;
}

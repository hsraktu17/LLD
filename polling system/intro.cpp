#include <iostream>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <ctime>
#include <string>
#include <cassert>

using namespace std;

struct Poll {
    string pollId;
    string question;
    vector<string> options;
    unordered_map<string, int> votes;
    time_t createdAt;
};

class PollManager {
private:
    unordered_map<string, Poll> polls;
    mutex mtx;
    int pollCounter = 0;
public:
    string createPoll(const string &question, const vector<string> &options) {
        lock_guard<mutex> lock(mtx);
        string pollId = to_string(++pollCounter);
        Poll poll = {pollId, question, options, {}, time(nullptr)};
        for (const auto &option : options) {
            poll.votes[option] = 0;
        }
        polls[pollId] = poll;
        return pollId;
    }
    
    bool updatePoll(const string &pollId, const string &question, const vector<string> &options) {
        lock_guard<mutex> lock(mtx);
        if (polls.find(pollId) == polls.end()) return false;
        polls[pollId].question = question;
        polls[pollId].options = options;
        polls[pollId].votes.clear();
        for (const auto &option : options) {
            polls[pollId].votes[option] = 0;
        }
        return true;
    }
    
    bool deletePoll(const string &pollId) {
        lock_guard<mutex> lock(mtx);
        return polls.erase(pollId) > 0;
    }
    
    Poll& getPoll(const string &pollId) {
        return polls[pollId];
    }
    
    bool pollExists(const string &pollId) {
        lock_guard<mutex> lock(mtx);
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
        if (!pollManager.pollExists(pollId)) return false;
        if (votes[pollId].find(userId) != votes[pollId].end()) return false; 
        
        Poll& poll = pollManager.getPoll(pollId);
        if (poll.votes.find(option) == poll.votes.end()) return false;
        
        votes[pollId][userId] = option;
        poll.votes[option]++;
        return true;
    }
    
    unordered_map<string, int> viewPollResults(const string &pollId) {
        if (!pollManager.pollExists(pollId)) return {};
        return pollManager.getPoll(pollId).votes;
    }
};

int main() {
    PollManager pollManager;
    VoteManager voteManager(pollManager);
    
    cout << "Creating Poll..." << endl;
    string pollId = pollManager.createPoll("What is your favorite color?", {"Red", "Blue", "Green", "Yellow"});
    cout << "Poll Created with ID: " << pollId << endl;
    Poll poll = pollManager.getPoll(pollId);
    cout << "Question: " << poll.question << endl;
    cout << "Options:" << endl;
    for (const auto &option : poll.options) {
        cout << "- " << option << endl;
    }
    
    cout << "Updating Poll..." << endl;
    if (pollManager.updatePoll(pollId, "What is your favorite season?", {"Spring", "Summer", "Autumn", "Winter"})) {
        cout << "Poll Updated Successfully." << endl;
        poll = pollManager.getPoll(pollId);
        cout << "Updated Question: " << poll.question << endl;
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
    unordered_map<string, int> results = voteManager.viewPollResults(pollId);
    for (const auto &[option, count] : results) {
        cout << option << ": " << count << " votes" << endl;
    }
    
    cout << "Deleting Poll..." << endl;
    if (pollManager.deletePoll(pollId)) {
        cout << "Poll Deleted Successfully." << endl;
    }
    
    return 0;
}

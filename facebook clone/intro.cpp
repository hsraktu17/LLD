#include<bits/stdc++.h>

using namespace std;

struct Post{
    int id;
    int userId;
    string content;
};

class Facebook{
private: 
    unordered_map<int, unordered_set<int>> followers;
    list<Post> posts;
    unordered_map<int, list<Post> :: iterator> PostMap;

public:
    void createPost(int userId, string content){
        int postId = posts.size() + 1;
        posts.push_front({postId, userId, content});
        PostMap[postId] = posts.begin();
    }

    void deletePost(int userId, int postId){
        if(PostMap.find(postId) != PostMap.end() && PostMap[postId]->userId == userId){
            posts.erase(PostMap[postId]);
            PostMap.erase(postId);
        }
    }

    void follow(int followerId, int followeeId){
        if(followerId != followeeId){
            followers[followerId].insert(followeeId);
        }
    }

    void unfollow(int followerId, int followeeId){
        if(followers[followerId].count(followeeId)){
            followers[followerId].erase(followeeId);
        }
    }


    vector<Post> getNewsFeed(int userId){
        vector<Post> feed;
        for(auto &post: posts){
            if(post.userId == userId || followers[userId].count(post.userId)){
                feed.push_back(post);
                if(feed.size() == 10) break;
            }
        }
        return feed;
    }
};

int main(){
    Facebook fb;

    int A = 1, B = 2, C = 3, D = 4;
    fb.follow(A,B);
    fb.follow(A,C);
    fb.follow(A,D);
    fb.createPost(A, "hello hello");
    fb.createPost(A, "Post from A - 1");
    fb.createPost(A, "Post from A - 2");
    fb.createPost(B, "Post from B");
    fb.createPost(C, "Post from C");
    fb.createPost(D, "Post from D");
    vector<Post> feed = fb.getNewsFeed(A);
    for (const auto& post : feed) cout << " User " << post.userId << " post " <<endl<<"content: "<< post.content<<endl;
    cout << endl;

    fb.unfollow(A,D);
    vector<Post> feed1 = fb.getNewsFeed(A);
    for (const auto& post : feed1) cout << " User " << post.userId << " post " <<endl<<"content: "<< post.content<<endl;
    cout << endl;
    return 0;
}

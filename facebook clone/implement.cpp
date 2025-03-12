#include<bits/stdc++.h>

using namespace std;

struct Post{
    int id;
    int userId;
    string content;
};

class Facebook{
private:
    unordered_map<int, unordered_set<int>>followers;
    list<Post> posts;
    unordered_map<int, list<Post> :: iterator> PostMap;


public:
    void createPost(int userId, string content){
        int postId = posts.size() + 1;
        posts.push_front({postId, userId, content});
        PostMap[postId] = posts.begin();
    }

    void deletePost(int userId, int postId){
        if(PostMap.find(postId) != PostMap.end() && PostMap[userId]->userId == userId){
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
        if(followerId != followeeId){
            followers[followerId].erase(followeeId);
        }
    }

    vector<Post> getNewsFrom(int userId){
        vector<Post> feed;
        for(auto post: posts){
            if(post.userId == userId || followers[userId].count(post.userId)){
                feed.push_back(post);
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
    fb.follow(B,C);

    fb.createPost(A, "Hello this is my second post on fb");
    fb.createPost(B, "Hello this is my second post on fb");
    fb.createPost(C, "Hello this is my second post on fb");
    fb.createPost(D, "Hello this is my second post on fb");
    fb.createPost(D, "Hello this is my second post on fb");
    fb.createPost(B, "Hello this is my second post on fb");

    vector<Post> feed = fb.getNewsFrom(A);
    vector<Post> feed1 = fb.getNewsFrom(B);
    
    for(auto post: feed) cout<< "User " << post.userId << " post "<<endl << " content: "<< post.content<<endl;
    cout<<"--------------------------"<<endl;
    for(auto post: feed1) cout<< "User " << post.userId << " post "<<endl<<" content: "<< post.content<<endl;
    return 0;
}

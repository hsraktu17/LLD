#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <memory>
using namespace std;

class MessageQueue {
private:
    unordered_map<string, queue<string>> topics;
    unordered_map<string, vector<void(*)(string, string)>> subscribers;

public:
    void publish(const string& topic, const string& message) {
        topics[topic].push(message);
        deliverMessages(topic);
    }

    void subscribe(const string& topic, void(*callback)(string, string)) {
        subscribers[topic].push_back(callback);
    }

    void deliverMessages(const string& topic) {
        while (!topics[topic].empty()) {
            string message = topics[topic].front();
            topics[topic].pop();

            if (subscribers.find(topic) != subscribers.end()) {
                for (auto& callback : subscribers[topic]) {
                    callback(message, topic);
                }
            }
        }
    }
};

class Producer {
private:
    string id;
    shared_ptr<MessageQueue> queue;

public:
    Producer(const string& id, shared_ptr<MessageQueue> queue) : id(id), queue(queue) {}
    void publish(const string& topic, const string& message) {
        cout << "[Producer " << id << "] Published: " << message << " to " << topic << endl;
        queue->publish(topic, message);
    }
};

class Consumer {
private:
    string id;

    static void consumeMessage(string message, string consumerId) {
        cout << consumerId << " received " << message << endl;
    }

public:
    Consumer(const string& id, shared_ptr<MessageQueue> queue, vector<string> topics) : id(id) {
        for (const auto& topic : topics) {
            queue->subscribe(topic, [](string message, string consumerId) {
                cout << consumerId << " received " << message << endl;
            });
        }
    }
};

int main() {
    auto queue = make_shared<MessageQueue>();

    // Creating topics
    string topic1 = "topic1";
    string topic2 = "topic2";

    // Creating producers
    Producer producer1("producer1", queue);
    Producer producer2("producer2", queue);

    // Creating consumers
    Consumer consumer1("consumer1", queue, {topic1, topic2});
    Consumer consumer2("consumer2", queue, {topic1});
    Consumer consumer3("consumer3", queue, {topic1, topic2});
    Consumer consumer4("consumer4", queue, {topic1, topic2});
    Consumer consumer5("consumer5", queue, {topic1});

    // Publish messages
    producer1.publish(topic1, "Message 1");
    producer1.publish(topic1, "Message 2");
    producer2.publish(topic1, "Message 3");
    producer1.publish(topic2, "Message 4");
    producer2.publish(topic2, "Message 5");

    return 0;
}

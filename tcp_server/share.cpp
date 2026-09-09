#include <iostream>
#include <set>
#include <map>

using namespace std;

class Share {
public:
	int connect_user(string id, int fd){
		if(user_in_use(id))
			return -1;

		users.insert({id, fd});
		cout << "inserted: " << fd << endl;
		return 0;
	}

	int get_fd(string id){
		try {
			return users.at(id);
		}catch(...) {
			return -1;
		}
	}

	int user_in_use(string id){
		if(users.find(id) == users.end())
			return 0;

		return users[id] >= 0;
	}
	
	int user_exists(string id){
		return users.find(id) != users.end();
	}

	void disconnect_user(string id){
		users[id] = -1;
	}

	void print_all_users(){
		for(auto it : users)
			cout << it.first << " ";
		cout << endl;
	}

	void print_active_users(){
		for(auto it : users)
			if(it.second != -1)
				cout << "(" << it.first << ":" << it.second << ") ";
		cout << endl;
	}

	int add_user_to_topic(string topic, string id){
		if(!user_exists(id))
			return -1;

		topics[topic].insert(id);
		return 0;
	}

	void print_all_topics(){
		for(auto it : topics){
			cout << it.first << ": ";
			for(auto user : it.second)
				cout << user << " ";
			cout << endl;
		}
	}

private:
	map<string, int> users;
	map<string, set<string>> topics;
};

int main(){
	Share s;
	s.connect_user("test", 1);
	s.connect_user("a", 5);
	s.connect_user("b", 3);
	s.connect_user("c", 7);
	s.print_active_users();
	s.add_user_to_topic("baba", "a");
	s.add_user_to_topic("baba", "b");
	s.add_user_to_topic("baba", "c");
	s.add_user_to_topic("caca", "c");
	s.print_all_topics();
}

#include <iostream>
#include <string>
#include <sys/types.h> // size_t, ssize_t
#include <sys/socket.h> // socket funcs
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // htons, inet_pton
#include <unistd.h> // close
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <vector>

using namespace std;


struct ThreadArgs {
	int clientSock; // socket to communicate with client
};


int main(int argc, char * argv[]){
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0) {
      cout << "Error with socket" << endl;
      exit(-1);
  }
	char *IPAddr = argv[1];
	int intPort = stoi(argv[2]);
	unsigned short servPort = static_cast<unsigned short>(intPort);
	unsigned long servIP = stoi(IPAddr);
	int status = inet_pton(AF_INET, IPAddr, (void *) &servIP);
	if (status <= 0) exit(-1);

	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET; // always AF_INET
	servAddr.sin_addr.s_addr = servIP;
	servAddr.sin_port = htons(servPort);
	status = connect(sock, (struct sockaddr *) &servAddr,
	sizeof(servAddr));
	if (status < 0) {
		cout << "Error with connect" << endl;
		exit(-1);
	}
	cout << "This is the cilent," << endl;
	string name;
	cout << "Please enter your name" << endl;
	cin >> name;
	long hostInt;
	hostInt = name.length();
	long networkInt = htonl(hostInt);
	int bytesSent = send(sock, (void *) &networkInt,
	sizeof(long), 0);
	if (bytesSent != sizeof(long)) {
        cerr << "Error sending name length" << endl;
        exit(-1);
    }
	// sending name legnth
	// Now sending name
	//char msg[hostInt];
	//strcpy(msg, name.c_str());
	const char *msg = name.c_str();
	bytesSent = send(sock, (void *) msg, hostInt, 0);
	if (bytesSent != hostInt) exit(-1);
	float distanceToTreasure = -1;
	int getX;
	int getY;
	while (distanceToTreasure != 0){
		cout << "Please enter the x cordinate of your guess: ";
		cin >> getX;
		while (-200 > getX || getX > 200){
			cout << "Invalid X cord. Please try again: ";
			cin >> getX;
		}
		cout << "Please enter the y cordinate of your guess: ";
		cin >> getY;
		while (-200 > getY || getY > 200){
			cout << "Invalid Y cord. Please try again: ";
			cin >> getY;
		}
		long hostIntX = getX;
		long hostIntY = getY;
		// Sends X and Y guess over the network to server
		long networkIntX = htonl(hostIntX);
		long networkIntY = htonl(hostIntY);
		int bytesSentX = send(sock, (void *) &networkIntX,
		sizeof(long), 0);
		if (bytesSentX != sizeof(long)) {
      		cerr << "Error sending name length" << endl;
      	exit(-1);
    	}
		int bytesSentY = send(sock, (void *) &networkIntY,
		sizeof(long), 0);
		if (bytesSentY != sizeof(long)) {
    	cerr << "Error sending name length" << endl;
      		exit(-1);
    }
		// This recv recives the distance in string format
		int bytesLeft = sizeof(long);
		long networkInt;
		// char * used because char is 1 byte
		char *bpLength = (char *) &networkInt;
		while (bytesLeft) {
			int bytesRecv = recv(sock, bpLength, bytesLeft, 0);
			if (bytesRecv <= 0) exit(-1);
			bytesLeft = bytesLeft - bytesRecv;
			bpLength = bpLength + bytesRecv;
		}
		long hostInt = ntohl(networkInt);
		bytesLeft = static_cast<int>(hostInt);
		vector<char> buffer(bytesLeft);
		char *bpFloat = buffer.data();
		while (bytesLeft){
			int bytesRecv = recv(sock,bpFloat,bytesLeft,0);
			if (bytesRecv <= 0){
				cerr << "Error receivng string length from server" << endl;
				exit(-1);
			}
			bytesLeft = bytesLeft - bytesRecv;
			bpFloat = bpFloat + bytesRecv;
		}
		string floatNum;
		floatNum.assign(buffer.data(),buffer.size());
		float distanceCount = stof(floatNum);
		distanceToTreasure = distanceCount;
		cout <<"Distance to Treasure: " << distanceToTreasure << endl;

	}
	// Next 2 receives are for the win message and number of turns
	int bytesLeft = sizeof(long);
	char *bpLength2 = (char *) &networkInt;
	while (bytesLeft) {
    	int bytesRecv = recv(sock, bpLength2, bytesLeft, 0);
    	if (bytesRecv <= 0) exit(-1);
    	bytesLeft -= bytesRecv;
    	bpLength2 += bytesRecv;
	}
	hostInt = ntohl(networkInt);

	// Allocate space for the win message
	vector<char> buffer(hostInt + 1);
	char *bpString = buffer.data();
	while (hostInt) {
    	int bytesRecv = recv(sock, bpString, hostInt, 0);
    	if (bytesRecv <= 0) {
        	cerr << "Error receiving win message from server" << endl;
        	exit(-1);
    	}
    	hostInt -= bytesRecv;
    	bpString += bytesRecv;
	}
// Null-terminate the received win message
	buffer[buffer.size() - 1] = '\0';

// Convert the received buffer to a string
	string winMessage(buffer.data());
	cout << winMessage << endl;
	// Final 2 recives
	// 1st one gets length of leaderboard string
	// 2nd one gets the leaderboard itself
	// Read the length of the leaderboard string

	int networkInt2;
	int bytesReceived = recv(sock, reinterpret_cast<char*>(&networkInt2), sizeof(int), 0);
	if (bytesReceived <= 0) {
    	cerr << "Error receiving leaderboard length from server" << endl;
    	exit(-1);
	}

// Convert network byte order to host byte order for length
	int hostInt2 = ntohl(networkInt2);

// Allocate buffer for leaderboard data (+1 for null terminator)
	vector<char> buffer2(hostInt2 + 1);

// Receive leaderboard data
	int totalBytesReceived = 0;
	char* bpLeaderboard = buffer2.data();
	while (totalBytesReceived < hostInt2) {
    	bytesReceived = recv(sock, bpLeaderboard + totalBytesReceived, hostInt2 - totalBytesReceived, 0);
    	if (bytesReceived <= 0) {
        	cerr << "Error receiving leaderboard data from server" << endl;
        	exit(-1);
    }
    	totalBytesReceived += bytesReceived;
}

// Null-terminate the received data
	buffer2[hostInt2] = '\0';

// Convert the received data to a string
	string leaderBoard(buffer2.data());

	cout << leaderBoard << endl;
	close(sock);

	return 0;


}
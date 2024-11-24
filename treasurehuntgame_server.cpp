#include <iostream>
#include <string>
#include <sys/types.h> // size_t, ssize_t
#include <sys/socket.h> // socket funcs
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // htons, inet_pton
#include <unistd.h> // close
#include <cstdlib>
#include <vector>
#include <cmath>
using namespace std;

const int MAXPENDING = 5;

void *threadMain(void *args);
void processClient(int clientSockNum);
string acesssLeaderBoard(string userName, int totalGueses);

// One function for the thread
// one to process the cilient
// and one to acess and change the leaderboard if needed before sending
// to cilient

struct ThreadArgs {
	int clientSock; // socket to communicate with client
};



struct leaderBoardEntry{
  string name;
  int turns;
  bool active;
  leaderBoardEntry(){
    active = false;
  }
};
// A leaderboard entry has 3 fields
// A name, turns
// And a field if it is active yet
// This is imporant for leaderboard checks and sending
// if not active, that means that does not have a score yet
// it will always start out inactive when intalized

vector<leaderBoardEntry> leaderboard(3);
pthread_mutex_t leaderboardMutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char * argv[]){
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    const char* IPAddr = "10.124.72.20";
    int intPort = stoi(argv[1]);
    unsigned short servPort = static_cast<unsigned short>(intPort);
		unsigned long servIP = stoi(IPAddr);
    int status = inet_pton(AF_INET, IPAddr, (void *) &servIP);
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET; // always AF_INET
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);
    status = bind(sock, (struct sockaddr *) &servAddr,
    sizeof(servAddr));
    if (status < 0) {
        cout << "Error with bind" << endl;
        exit(-1);
    }
    status = listen(sock, MAXPENDING);
    if (status < 0) {
      cout << "Error with listen" << endl;
      exit(-1);
    }
    while (true) {
// Accept connection from client
    	struct sockaddr_in clientAddr;
    	socklen_t addrLen = sizeof(clientAddr);
    	int clientSock = accept(sock,(struct sockaddr *) &clientAddr,
    	&addrLen);
    	if (clientSock < 0) exit(-1);
    	// Create and initialize argument struct
    	ThreadArgs *args = new ThreadArgs;
    	args -> clientSock = clientSock;
    	// Create client thread
    	pthread_t threadID;
    	int status = pthread_create(&threadID, NULL, threadMain,
    	(void *) args);
    	if (status != 0) exit(-1); // Note: status == 0 is GOOD
    }
    return 0;


}

void *threadMain(void *args)
{
    // Extract socket file descriptor from argument
    struct ThreadArgs *threadArgs = (struct ThreadArgs *) args;
    int clientSock = threadArgs->clientSock;
    delete threadArgs;
    // Communicate with client
    processClient(clientSock);
    // Reclaim resources before finishing
    pthread_detach(pthread_self());
    close(clientSock);
    return NULL;
}

void processClient(int clientSockNum){
  int bytesLeft = sizeof(long);
  long networkInt;
  string userName;
  // char * used because char is 1 byte
  // This is used to read the string length
  char *bp = (char *) &networkInt;
  while (bytesLeft) {
    int bytesRecv = recv(clientSockNum, bp, bytesLeft, 0);
    if (bytesRecv <= 0) {
      cerr << "Error receiving data from client" << endl;
      close(clientSockNum); // Close the client socket
      return; // Exit the function gracefully
      }
    bytesLeft = bytesLeft - bytesRecv;
    bp = bp + bytesRecv;
  }
  long hostInt = ntohl(networkInt);

  bytesLeft = static_cast<int> (hostInt);// bytes to read
  vector<char> buffer(hostInt); // initially empty
  bp = buffer.data();
  while (bytesLeft) {
    int bytesRecv = recv(clientSockNum, bp, bytesLeft, 0);
      if (bytesRecv <= 0) {
        cerr << "Error receiving string length from client" << endl;
        close(clientSockNum); // Close the client socket
        return; // Exit the function gracefully
      }
    bytesLeft = bytesLeft - bytesRecv;
    bp = bp + bytesRecv;
  }
  userName.assign(buffer.data(), buffer.size());
  // Assings the username the buffer
  srand(time(0));
  int targetCordX = (rand() % 400) - 200;
  int targetCordY = (rand() % 400) - 200;
  // generates x and y cords
  //cout << targetCordX << " " << targetCordY << endl;
  float distanceToTreasure = -1;
  int guessX;
  int guessY;
  int totalGueses = 0;
  while (distanceToTreasure != 0.0){
    int bytesLeftX = sizeof(long);
    long networkIntX;
    char *bpX = (char *) &networkIntX;
    while (bytesLeftX) {
      // Recives an X guess
      int bytesRecv = recv(clientSockNum, bpX, bytesLeftX, 0);
      if (bytesRecv <= 0) {
        cerr << "Error receiving data from client" << endl;
        close(clientSockNum); // Close the client socket
        return; // Exit the function gracefully
        }
      bytesLeftX = bytesLeftX - bytesRecv;
      bpX = bpX + bytesRecv;
    }
    long hostIntX = ntohl(networkIntX);
    // Recives a Y guess
    int bytesLeftY = sizeof(long);
    long networkIntY;
    char *bpY = (char *) &networkIntY;
    while (bytesLeftY) {
      int bytesRecv = recv(clientSockNum, bpY, bytesLeftY, 0);
      if (bytesRecv <= 0) {
        cerr << "Error receiving data from client" << endl;
        close(clientSockNum); // Close the client socket
        return; // Exit the function gracefully
        }
      bytesLeftY = bytesLeftY - bytesRecv;
      bpY = bpY + bytesRecv;
    }
    long hostIntY = ntohl(networkIntY);
    guessX = static_cast<int>(hostIntX);
    guessY = static_cast<int>(hostIntY);
    //cout << guessX << " " << guessY << endl;
    // checks distance
    if (guessX == targetCordX && guessY == targetCordY){
      distanceToTreasure = 0.0;
    }else{
      float squareDistance = pow(guessX - targetCordX, 2) + pow(guessY - targetCordY, 2);
      distanceToTreasure = sqrt(squareDistance);
    }
    // sends distance back to client
    // This is done by sending as a STRING
    // cout << distanceToTreasure << endl;
    string sendDecimal = to_string(distanceToTreasure);
    long hostInt = sendDecimal.length();
    long networkInt = htonl(hostInt);
    int bytesSent = send(clientSockNum, (void *) &networkInt,
	  sizeof(long), 0);
    //cout << bytesSent << endl;
	  if (bytesSent != sizeof(long)) {
        cerr << "Error sending name length" << endl;
        close(clientSockNum);
    }
    const char *msg = sendDecimal.c_str();
	  bytesSent = send(clientSockNum, (void *) msg, hostInt, 0);
	  if (bytesSent != hostInt) close(clientSockNum);
    totalGueses++;

  }
  // win message and number of turns


  string sendWin = "Congratulations! You found the treasure!\n";
  sendWin += "It took " + to_string(totalGueses) + " turns to find the treasure.\n";

// Get the length of the win message
  long hostIntsendWin = sendWin.length();
  long networkIntsendWin = htonl(hostIntsendWin);

// Send the length of the win message
  int bytesSent = send(clientSockNum, (void *) &networkIntsendWin, sizeof(long), 0);
  if (bytesSent != sizeof(long)) {
    cerr << "Error sending win message length" << endl;
    close(clientSockNum);
  }

// Send the win message itself
  const char *msg = sendWin.c_str();
  bytesSent = send(clientSockNum, (void *) msg, hostIntsendWin, 0);
  if (bytesSent != hostIntsendWin) {
    cerr << "Error sending win message" << endl;
    close(clientSockNum);
  }
  // once cilient finds location, it acesses string leaderboard
  string sendString = acesssLeaderBoard(userName, totalGueses);
  // the leaderboard is sent as a long string
  cout << sendString << endl;
// Get the length of the leaderboard string
  int hostInt2 = sendString.length();
  int networkInt2 = htonl(hostInt2);

// Send the length of the leaderboard string
  bytesSent = send(clientSockNum, (void *) &networkInt2, sizeof(int), 0);
  if (bytesSent != sizeof(int)) {
    cerr << "Error sending leaderboard length" << endl;
    close(clientSockNum);
  }

// Send the leaderboard data
  const char *msg2 = sendString.c_str();
  bytesSent = send(clientSockNum, (void *) msg2, hostInt2, 0);
  if (bytesSent != hostInt2) {
    cerr << "Error sending leaderboard data" << endl;
    close(clientSockNum);
  }

}

string acesssLeaderBoard(string userName, int totalGueses){
  // updates the leaderboard and sends back the leaderboard to sent to the cilient
  // deals with MANY edge cases(including having less then 3 entires)
  pthread_mutex_lock(&leaderboardMutex);
  if (leaderboard[0].active == false && leaderboard[1].active == false && leaderboard[2].active == false){
    leaderboard[0].name = userName;
    leaderboard[0].turns = totalGueses;
    leaderboard[0].active = true;
  }else if((leaderboard[0].active == true && leaderboard[1].active == false && leaderboard[2].active == false)){
    if (totalGueses < leaderboard[0].turns){
      leaderboard[1].name = leaderboard[0].name;
      leaderboard[1].turns = leaderboard[0].turns;
      leaderboard[1].active = true;
      leaderboard[0].name = userName;
      leaderboard[0].turns = totalGueses;
    }else{
      leaderboard[1].name = userName;
      leaderboard[1].turns = totalGueses;
      leaderboard[1].active = true;
    }
  }else if((leaderboard[0].active == true && leaderboard[1].active == true && leaderboard[2].active == false)){
     if (totalGueses < leaderboard[0].turns){
      leaderboard[2].name = leaderboard[1].name;
      leaderboard[2].turns = leaderboard[1].turns;
      leaderboard[2].active = true;
      leaderboard[1].name = leaderboard[0].name;
      leaderboard[1].turns = leaderboard[0].turns;
      leaderboard[0].name = userName;
      leaderboard[0].turns = totalGueses;
     }else if(totalGueses >= leaderboard[0].turns && totalGueses < leaderboard[1].turns){
      leaderboard[2].name = leaderboard[1].name;
      leaderboard[2].turns = leaderboard[1].turns;
      leaderboard[2].active = true;
      leaderboard[1].name = userName;
      leaderboard[1].turns = totalGueses;
     }else{
      leaderboard[2].name = userName;
      leaderboard[2].turns = totalGueses;
     }

  }else{
    if (totalGueses < leaderboard[0].turns){
      leaderboard[2].name = leaderboard[1].name;
      leaderboard[2].turns = leaderboard[1].turns;
      leaderboard[1].name = leaderboard[0].name;
      leaderboard[1].turns = leaderboard[0].turns;
      leaderboard[0].name = userName;
      leaderboard[0].turns = totalGueses;
    }else if(totalGueses >= leaderboard[0].turns && totalGueses < leaderboard[1].turns){
      leaderboard[2].name = leaderboard[1].name;
      leaderboard[2].turns = leaderboard[1].turns;
      leaderboard[1].name = userName;
      leaderboard[1].turns = totalGueses;
    }else if(totalGueses >= leaderboard[1].turns && totalGueses < leaderboard[2].turns){
      leaderboard[2].name = userName;
      leaderboard[2].turns = totalGueses;

    }

  }
  string leaderBoardString = "";
  leaderBoardString.append("Leaderboard: \n");
  for (int i = 0; i < (int)leaderboard.size(); i++){
    if(leaderboard[i].active == true){
      leaderBoardString.append(to_string(i+1));
      leaderBoardString.append(". ");
      leaderBoardString.append(leaderboard[i].name);
      leaderBoardString.append(" ");
      leaderBoardString.append(to_string(leaderboard[i].turns));
      leaderBoardString.append(" \n");
    }
  }
  pthread_mutex_unlock(&leaderboardMutex);
  return leaderBoardString;

}


  
    



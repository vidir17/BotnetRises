//
// Simple chat server for TSAM-409
//
// Command line: ./chat_server 4000 
//
// Author: Jacky Mallett (jacky@ru.is)
//
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>

#include <iostream>
#include <sstream>
#include <thread>

#include <unistd.h>

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG  5          // Allowed length of queue of waiting connections
using namespace std;
int counter = 0;
// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Server
{
    public:
    int sock;
    std::string name; //server name

    Server(int socket) : sock(socket){}

    ~Server(){}
};
class Client
{
  public:
    int sock;              // socket of client connection
    std::string name;           // Limit length of name of client's user

    Client(int socket) : sock(socket){} 

    ~Client(){}            // Virtual destructor defined for base class
};

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table, 
// (indexed on socket no.) sacrificing memory for speed.

static std::map<int, Client*> clients; // Lookup table for per Client information
static std::map<int, Server*> servers; // Lookup table for per Server information

// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.
int open_socket(int portno)
{
   struct sockaddr_in sk_addr;   // address settings for bind()
   int sock;                     // socket opened for this port
   int set = 1;                  // for setsockopt

   // Create socket for connection. Set to be non-blocking, so recv will
   // return immediately if there isn't anything waiting to be read.
#ifdef __APPLE__     
   if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      perror("Failed to open socket");
      return(-1);
   }
#else
   if((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
   {
     perror("Failed to open socket");
    return(-1);
   }
#endif

   // Turn on SO_REUSEADDR to allow socket to be quickly reused after 
   // program exit.

   if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
   {
      perror("Failed to set SO_REUSEADDR:");
   }
   set = 1;
#ifdef __APPLE__     
   if(setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0)
   {
     perror("Failed to set SOCK_NOBBLOCK");
   }
#endif
   memset(&sk_addr, 0, sizeof(sk_addr));

   sk_addr.sin_family      = AF_INET;
   sk_addr.sin_addr.s_addr = INADDR_ANY;
   sk_addr.sin_port        = htons(portno);

   // Bind to socket to listen for connections from clients

   if(bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
   {
      perror("Failed to bind to socket:");
      return(-1);
   }
   else
   {
      return(sock);
   }
}

void AddToServerList(string IP, string PORT){
    

    std::string address;

    address = IP;
    address += ":";
    address += PORT;
    //add server list  
    
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds)
{
     // Remove client from the clients list
     string name_of = clients[clientSocket]->name;
     cout << "Bye bye " << name_of <<  endl;
     clients.erase(clientSocket);
     cout << name_of << " Disconnected..." << endl;
     cout << "Online clients remaining " << clients.size() << endl;
     

     // If this client's socket is maxfds then the next lowest
     // one has to be determined. Socket fd's can be reused by the Kernel,
     // so there aren't any nice ways to do this.
     if(*maxfds == clientSocket)
     {
        for(auto const& p : clients)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
     }

     // And remove from the list of open sockets.
     FD_CLR(clientSocket, openSockets);

}
void connectHere(string a, string b){
    int socket_ = socket(AF_INET, SOCK_STREAM, 0); //IPv4 Internet Protocol & SOCK_STREAM for stream of data

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;//specify family of address
    server_address.sin_addr.s_addr = inet_addr(a.c_str());//Address on next serve
    
    //PASSA AD THAD MA EKKI TENGJAST SEM CLIENT BARA SERVER TO SERVER
    server_address.sin_port = htons(stoi(b));//port

    int address_size = sizeof(server_address);
    int errorHandling = connect(socket_, (struct sockaddr*)&server_address, address_size);


    if(errorHandling < 0){
        cout << "No connection established to server" << endl;
            }else{
        cout << "Connected to server" << endl;
        AddToServerList(a, b);

    }
}
// Get name from client on the server
void clientCommandName(int clientSocket, fd_set *openSockets, int *maxfds, 
                  char *buffer)
{
               
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream stream(buffer);
  while(stream >> token){
      tokens.push_back(token);
    }
    
    clients[clientSocket]->name = tokens[0];
    cout << "Welcome " << tokens[0] << "!" << endl;


}


// Process command from client on the server
void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, 
                  char *buffer) 
{
  std::vector<std::string> tokens;
  std::string token;

  // Split command from client into tokens for parsing
  std::stringstream stream(buffer);

  while(stream >> token)
      tokens.push_back(token);

  if((tokens[0].compare("CONNECT") == 0) && (tokens.size() == 3))
  {
    
     cout << "Here you will connect" << endl;
     cout << "CONNECT: " << tokens[0] << endl;
     cout << "IP: " << tokens[1] << endl;
     cout << "PORT: " << tokens[2] << endl;
     //cout << "Name: " << servers[server->sock]->name << endl;
    
     connectHere(tokens[1], tokens[2]);

     //cout << clients[clientSocket]->name << endl;
     //cout << 

     
  }
  else if(tokens[0].compare("LEAVE") == 0)
  {
      // Close the socket, and leave the socket handling
      // code to deal with tidying up clients etc. when
      // select() detects the OS has torn down the connection.
      cout << "get out" << endl;
      closeClient(clientSocket, openSockets, maxfds);
    
  }
  else if(tokens[0].compare("WHO") == 0)
  {
     std::cout << "Who is logged on" << std::endl;
     std::string msg;

     for(auto const& names : clients)
     {
        msg += names.second->name + ",";
     }
     // Reducing the msg length by 1 loses the excess "," - which
     // granted is totally cheating.
     cout << msg.c_str() << endl;
     send(clientSocket, msg.c_str(), msg.length()-1, 0);

  }
  else if(tokens[0].compare("LISTSERVERS") == 0){
      std::cout << "List of servers connected: " << std::endl;
     std::string msg;
     send(clientSocket, "Servers:", 9, 0);
     for(auto const& names : servers)
     {
        //send(clientSocket, "Servers:", 9, 0);
        msg += names.second->name; //laga
     }
     send(clientSocket, msg.c_str(), msg.length()-1, 0);
  }
  // This is slightly fragile, since it's relying on the order
  // of evaluation of the if statement.
  else if((tokens[0].compare("MSG") == 0) && (tokens[1].compare("ALL") == 0))
  {
      std::string msg;
      for(auto i = tokens.begin()+2;i != tokens.end();i++) 
      {
          msg += *i + " ";
      }

      for(auto const& pair : clients)
      {
          send(pair.second->sock, msg.c_str(), msg.length(),0);
      }
  }
  else if(tokens[0].compare("MSG") == 0)
  {
      for(auto const& pair : clients)
      {
          if(pair.second->name.compare(tokens[1]) == 0)
          {
              std::string msg;
              for(auto i = tokens.begin()+2;i != tokens.end();i++) 
              {
                  msg += *i + " ";
              }
              send(pair.second->sock, msg.c_str(), msg.length(),0);
          }
      }
  }
  else
  {
      if(counter !=0){
      std::cout << "Unknown command from client:" << buffer << std::endl;
    }
  }
     
}

int main(int argc, char* argv[])
{
    bool finished;
    int listenSock, listenSockClient;                 // Socket for connections to server
    int clientSock;                 // Socket of connecting client
    int serverSock;
    fd_set openSockets;             // Current open sockets 
    //fd_set openSocketsClients;
    fd_set readSockets;             // Socket list for select()    
    //fd_set readSocketsClients;    
    fd_set exceptSockets;// Exception socket list
    //fd_set exceptSocketsClients;           
    int maxfds;                     // Passed to select() as max fd in set
   // int maxfdsClients;
    //int clientPort;
    //struct sockaddr_in client;
    socklen_t clientLen;
    socklen_t serverLen;
    char buffer[1025];              // buffer for reading from clients
    if(argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }
    

    //Setup client sock for server to listen to, find next available port
    listenSockClient = open_socket(atoi(argv[1]) + 1);
    printf("Listening for clients on port: %d\n", atoi(argv[1]) + 1);
    
    if(listen(listenSockClient, BACKLOG) < 0)
    {
        printf("Listen failed on client port %s\n", argv[1]);
        exit(0);
    }
    else{
        // Add listen socket to socket set we are monitoring
        FD_ZERO(&openSockets);
        FD_SET(listenSockClient, &openSockets);
        maxfds = listenSockClient;
    }


// Setup server socket for server to listen to other servers

    listenSock = open_socket(atoi(argv[1]));
    printf("Listening for servers on port: %d\n", atoi(argv[1]));
    
    if(listen(listenSock, BACKLOG) < 0)
    {
        printf("Listen failed on server port %s\n", argv[1]);
        exit(0);
    }
    else 
    // Add listen socket to socket set we are monitoring
    {
        FD_SET(listenSock, &openSockets);
        if(listenSock > listenSockClient){
            maxfds = listenSock;
        }
        cout << "opensockets: " << &openSockets << endl;
    }
    
    
    


    finished = false;

    while(!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        //readSocketsClients = exceptSocketsClients = openSocketsClients;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);
        //  int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if(n < 0)
        {
            perror("select failed - closing down\n");
            finished = true;
        }
        else
        {
            Client *client;
            Server *server;
            //int clientSock = accept(listenSock, (struct sockaddr *)&client);
                     //              &clientLen);
            //send(clientSock2, "13", 2,0);            
            //cin >> listenSockClient;
            // First, accept  any new connections to the server on the listening socket
            //cout << "Read sockets: " << &readSocketsClient << endl;
            if(FD_ISSET(listenSockClient, &readSockets))
            {
            
               clientSock = accept(listenSockClient, (struct sockaddr *)&client,
                                  &clientLen);
               printf("accept***\n");
                // Add new client to the list of open sockets
                FD_SET(clientSock, &openSockets);
               // And update the maximum file descriptor
               maxfds = std::max(maxfds, clientSock);

               // create a new client to store information.
               clients[clientSock] = new Client(clientSock);           

               // Decrement the number of sockets waiting to be dealt with
               n--;
               send(clientSock, "Add Name: ", 10, 0);
               printf("Client connected on server: %d\n", clientSock);
            }
            if(FD_ISSET(listenSock, &readSockets))
            {
            
               serverSock = accept(listenSock, (struct sockaddr *)&server, &serverLen);
               printf("accept***\n");
                // Add new client to the list of open sockets
                FD_SET(serverSock, &openSockets);
                //FD_SET(serverSock, &openSockets);

               // And update the maximum file descriptor
               maxfds = std::max(maxfds, serverSock) ;
               
               // create a new server to store information.
               servers[serverSock] = new Server(serverSock);
               //add
               // Decrement the number of sockets waiting to be dealt with
               n--;
               send(serverSock, "Add Name: ", 10, 0);
               printf("Server connected to server: %d\n", serverSock);
            }
            // Now check for commands from clients
            while(n-- > 0)
            {
               for(auto const& pair : clients)
               {
                  Client *client = pair.second;
                  
                  if(FD_ISSET(client->sock, &readSockets))//goes here if commands from client
                  {
                  
                     
                      // recv() == 0 means client has closed connection
                      if(recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                      {
                          printf("Client closed connection: %d", client->sock);
                          close(client->sock);

                          closeClient(client->sock, &openSockets, &maxfds);
                          
                          break;
                          

                      }
                      // We don't check for -1 (nothing received) because select()
                      // only triggers if there is something on the socket for us.
                      else
                      {
                          std::cout << buffer << std::endl;
                          
                            //Add name here
                          while(clients[client->sock]->name == ""){
                            //added name goes in the list
                          clientCommandName(clients[clientSock]->sock, &openSockets, &maxfds, 
                                        buffer);
                                        

                      }

                          clientCommand(client->sock, &openSockets, &maxfds, 
                                        buffer);
                                        if(clients.size() == 0){
                                            close(client->sock);
                                            break;
                                        }
                                        if(counter == 0){
                                          counter++;
                            }
                      }
                  }
               }
            }
        }
    }//
}

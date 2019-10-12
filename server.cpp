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
int counter, counter2 = 0;
static int NewestServerSocket;
// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Message
{
    public:
    std::string GroupID;
    std::vector<std::string> storedMessage;

    Message(string GID) : GroupID(GID){}

    ~Message(){}
};

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
static std::map<string, Message*> message; // Lookup table for per Message information

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








void AddToServerList(int serverSocket, fd_set *openSockets, int *maxfds, 
                        string IP, string PORT){
    //readSockets = exceptSockets = openSockets;
    //for(auto const& pair : servers)
      //         {
                  
    std::string address = "";

    //std::vector<std::string> tokens2;
    

    
    
    address += IP;
    address += ",";
    address += PORT;
    address += ";";

    
    //tokens2.push_back(address);
   // cout << "tokens 0: " << tokens2[0] << endl;
    //FIND SERVER SOCKET
   // cout << serverSocket << " seeeeeeeeeeee" << endl;
    servers[serverSocket] = new Server(serverSocket);
    if (counter2 == 0){
        servers[serverSocket]->name = "SERVERS,";
        servers[serverSocket]->name += "P3_GROUP_91,";
        servers[serverSocket]->name += address;
        servers[serverSocket]->sock = serverSocket;
        
        counter2++;
    }else{
    servers[serverSocket]->name = "P3_GROUP_91,";
    servers[serverSocket]->name += address;
    servers[serverSocket]->sock = serverSocket;
    cout << "Here am I " << endl;
    cout << "ip: " << IP << endl;
        cout << "port: " << PORT << endl;
    if(IP != "85.220.73.127"){
        
        send(serverSocket, "hi", 2, 0);
    }
    }
}

void closeServer(int serverSocket, fd_set *openSockets, int *maxfds)
{
    string name_of = servers[serverSocket]->name;
    servers.erase(serverSocket);
    name_of[0] = '\0';
    cout << name_of << " disconnected..." << endl;
    cout << "online servers remaining " << servers.size() << endl;
    if(*maxfds == serverSocket){
        for(auto const& p: servers){
            *maxfds = std::max(*maxfds, p.second->sock);
        }
    }
    FD_CLR(serverSocket, openSockets);
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
bool connectHere(fd_set *openSockets, int clientSocket, int *maxfds, string a, string b){//EYDA UTy
    int socket_ = socket(AF_INET, SOCK_STREAM, 0); //IPv4 Internet Protocol & SOCK_STREAM for stream of data

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;//specify family of address
    server_address.sin_addr.s_addr = inet_addr(a.c_str());//Address on next serve
    
    int result = inet_pton(AF_INET, a.c_str(), &(server_address.sin_addr));
    cout << "Result: " << result << endl;
    if(result == 0){
        return 0;
    }
    //PASSA AD THAD MA EKKI TENGJAST SEM CLIENT BARA SERVER TO SERVER
    server_address.sin_port = htons(stoi(b));//port

    int address_size = sizeof(server_address);
    int errorHandling = connect(socket_, (struct sockaddr*)&server_address, address_size);
    cout << "servEr: " << &(server_address.sin_addr) << endl;

    if(errorHandling < 0){
        cout << "No connection established to server" << endl;
            }else{
        cout << "Connected to server" << endl;
        
        AddToServerList(socket_, openSockets, maxfds, a, b);
        
        
    }
    return 1;
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
    
     if(connectHere(openSockets, clientSocket,maxfds, tokens[1], tokens[2]) == 0){
         send(clientSocket, "Connecting IP failed...you can try again", 38, 0);
     }else{
        
         send(clientSocket, "Server connection made to ", 26, 0);
         send(clientSocket, tokens[1].c_str(), tokens[1].size(), 0);
         send(clientSocket, " ", 1, 0);
         send(clientSocket, tokens[2].c_str(), tokens[2].size(), 0);
    }
     //cout << clients[clientSocket]->name << endl;
     //cout << 

     
//    serverC(tokens[1], tokens[2]);

  }
  else if(tokens[0].compare("LEAVE") == 0)
  {
      // Close the socket, and leave the socket handling
      // code to deal with tidying up clients etc. when
      // select() detects the OS has torn down the connection.
      send(clientSocket, "get out!", 7, 0);
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
     cout << "this is the LIST: \n" << msg.c_str() << endl;
     send(clientSocket, msg.c_str(), msg.length()-1, 0);
  }
  //SENDMSG, GROUP ID
  else if(tokens[0].compare("SENDMSG,") == 0)
  {
      std::string msg;
      string GroupID = tokens[1];
      cout << tokens[1] << endl;
      message[GroupID] = new Message(GroupID);
      message[GroupID]->GroupID = tokens[1];
      for(unsigned int i = 2; i < tokens.size(); i++){
        message[GroupID]->storedMessage.push_back(tokens[i]);
      }
      
      cout << "THIS IS THE SIZE OF TOKENS: " << tokens.size();

        ///////////TEST/////////////
      cout << "Group ID: " << message[GroupID]->GroupID << "\n" << "The test: ";
       for(unsigned int i = 0; i < tokens.size()-2; i++){
      cout <<  message[GroupID]->storedMessage[i];

      }
  }
  else if(tokens[0].compare("GETMSG,") == 0) //Hægt að bæta, ef það er til group id
  {
      std::string msg;
      //get by group id
      for(auto const & pair : message)
      {
          //if(pair.second->GroupID.compare(tokens[1])){
            //  msg += 
          //


         if(pair.second->GroupID == tokens[1]){

            for (std::vector<string>::iterator it = pair.second->storedMessage.begin() ; it != pair.second->storedMessage.end(); ++it)
         {
             msg += *it + " ";
         } 
         send(clientSocket, msg.c_str(), msg.length()-1, 0);
                
                //std::cout << ' ' << *it;
                      //  std::cout << '\n';








          //   for(unsigned i = 2; i < 5; i++){
             //cout << "abcd" << endl;
             
            //    cout << "here: " << pair.second->storedMessage.rbegin() << endl;
            // }
             //msg += message[tokens[1]]->storedMessage;
     //   for(unsigned int i = 2; i < tokens.size(); i++){
       //     message[GroupID]->storedMessage.push_back(tokens[i]);
      }
        // cout << "FOR LOOP GROUP ID" << endl;
         //}
         //loop though vector
         //for(unsigned int i = 0; i < message[clientsocket]){
         //msg += message[clientSocket]->storedMessage[i]
         //}
      }
      //send(clientSocket, msg)


      //when group id found

      //print all messages with groupid in front

      
  }


  // This is slightly fragile, since it's relying on the order
  // of evaluation of the if statement.
  else if((tokens[0].compare("MSG") == 0) && (tokens[1].compare("ALL") == 0))
  {
      cout << "MSG all!!! " << endl;
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
  else if(tokens[0].compare("GET MSG") == 0)
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
    NewestServerSocket = listenSockClient;
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
AddToServerList(NewestServerSocket, &openSockets, &maxfds, "85.220.73.127", argv[1]);// laga i current port
        
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
               cout << "INFO: " << listenSock << "\n" << (struct sockaddr *)&server << endl;
               printf("accept***\n");
                // Add new server to the list of open sockets
                FD_SET(serverSock, &openSockets);
                cout << "server buffer: " << buffer << endl;
              

               // And update the maximum file descriptor
               maxfds = std::max(maxfds, serverSock) ;
               
               // create a new server to store information.
               //servers[serverSock] = new Server(serverSock); ////KANNSKI HAFA?
               //add
               
               //AddToServerList(serverSock, &openSockets, &maxfds, argv[0], argv[1]);
               // Decrement the number of sockets waiting to be dealt with
               n--;
               
               send(serverSock, "", 10, 0);

               printf("Server connected to server: %d\n", serverSock);
            }
            for(auto const& pair : servers)
               {
                  Server *server = pair.second;
                  // recv() == 0 means client has closed connection
                      if(recv(server->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                      {
                          cout << "this is the dog" << endl;
                          printf("Server closed connection: %d", server->sock);
                          close(server->sock);

                          closeServer(server->sock, &openSockets, &maxfds);
                          //send(server->sock, "bingobingo", 10, 0);
                          break;
                          

                      }else{
                         // cout << "This is the cat" << endl;
                      //    cout << "HEEEEEEE" << endl;
                      //std::cout << "Buffer: " << buffer << std::endl;
                      //break;
                      }
               
               
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
                          std::cout << buffer << std::endl; //server cout
                          
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
                                        if(counter == 1){
                                          counter--;
                                        }
                      }
                  }
               }
            }
        }
    }//
}

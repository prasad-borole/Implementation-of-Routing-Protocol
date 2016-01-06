/*
 ============================================================================
 Name        : assignment2.c
 Author      : Prasad Borole
 Version     : 1.0
 Copyright   : 
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>

#define LENGTH 512
#define MAXBUFLEN 512
#define inf 65535

static int updateTime=1;
char filename[100];
static int server_num=0;
static int neigh_num=0;
static int packet_counter=0;
static char myIPaddr[LENGTH];
static char myPort[LENGTH];
static int myConnId=0;
static int recvNum=0;
static char recvIPaddr[LENGTH];
static char recvPort[LENGTH];
static int recvId=0;
static char updateCounter[10][10];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//server_table DATA-STRUCTURE IS USED TO STORE SEVRER-IP-PORT INFORMATION.
//IT IS ALSO USED AS TO SEND UPDATE MESSAGES TO NEIGHBORS (DVs). Refer Page.2 of report(pborole_proj2.pdf)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct server_table{
    int server_id;
    char server_ip[100];
    char server_port[100];
    
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//THIS DATA-STRUCTURE HOLDS CURRENT DISTANCE ROUTING TABLE INFORMATION. Refer Page.2 of report(pborole_proj2.pdf)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct dist_table{
    int host_id;
    int dest_id;
    int next_hop;
    unsigned int cost;
    
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//THIS STRUCT STORES NEIGHBOR'S INFORMATION. Refer Page.2 of report(pborole_proj2.pdf)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct neigh_table{
    int dest_id;
};

struct server_table server_obj[10];
struct dist_table dist_obj[10];
struct dist_table dist_file_obj[10];
struct dist_table temp_obj[10];
struct neigh_table neigh_obj[10];

void error_msg(const char *msg)
{
	perror(msg);
	exit(1);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//The get_in_addr function is used for initiaize=ing UDPsocket and sending/receiving UDP message.
//It is inspired by beej Programming guide
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void getMyIP() {
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//UDP socket is created and it connects to Google DNS server 8.8.8.8. 
//The udp packet received from DNS is used to identify IP of the host.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char myhost_name[128];
gethostname(myhost_name, sizeof myhost_name);
//printf("My hostname: %s\n", myhost_name);

	const char* kGoogleDnsIp = "8.8.8.8";
	uint16_t kDnsPort = 53;
	struct sockaddr_in serv;
	struct sockaddr_in name;
	socklen_t namelen;
	int sock, err;
	char *ip; 
	
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		error_msg("sock error.");

	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
	serv.sin_port = htons(kDnsPort);

	err = connect(sock, (struct sockaddr *) &serv, sizeof(serv));
	if (err < 0)
		error_msg("connect error");		

	namelen = sizeof(name);
	err = getsockname(sock, (struct sockaddr *) &name, &namelen);
	if (err)
		error_msg("error");		

	 ip = (char *) inet_ntop(AF_INET, &name.sin_addr, myIPaddr, sizeof myIPaddr);
	if (!ip)
		error_msg("error");
	
	//printf("IP: %s\n", myIPaddr);
	
	close(sock);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Retrieve the IP address port and connection id of host.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    int i=0;
    for(i=0;i<server_num;i++)
        if(!strcasecmp(myIPaddr,server_obj[i].server_ip)) {
        break;
        }
    strcpy(myPort,server_obj[i].server_port);
    myConnId=server_obj[i].server_id;

/*
    strcpy(myIPaddr,server_obj[0].server_ip);
    strcpy(myPort,server_obj[0].server_port);
    myConnId=server_obj[0].server_id;
    */
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Initiaize Distance Vector Table
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void initiaizeDistTable() {
    int i=0,j=0;
    printf("My connection id:%d\n",myConnId);
    for(i=0;i<server_num;i++) {

    if(server_obj[i].server_id==myConnId)
    continue;

    dist_obj[j].host_id=myConnId;
    dist_obj[j].dest_id=server_obj[i].server_id;
    dist_obj[j].next_hop=dist_obj[j].dest_id;
    dist_obj[j].cost=inf;
    
    //printf("dest_id:%d->%d\n",server_obj[i].server_id,dist_obj[j].dest_id);
    j++;
    }

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Update distance vector table based on topology file.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void updateDistTable() {
    int i=0,k=0;
    for(i=0;i<server_num;i++){
        for(k=0;k<neigh_num;k++) {
        if(dist_obj[i].dest_id==dist_file_obj[k].dest_id) {
        dist_obj[i].host_id=dist_file_obj[k].host_id;
        dist_obj[i].dest_id=dist_file_obj[k].dest_id;
        dist_obj[i].next_hop=dist_file_obj[k].next_hop;
        dist_obj[i].cost=dist_file_obj[k].cost;
        }
        }
    }

    for(i=0;i<server_num-1;i++) {
        dist_file_obj[i]=dist_obj[i];
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Display Distance Vector table of host.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void displayDistTable() {
    int i=0;
    //printf("neigh_num:%d\n",neigh_num);
    printf("%-22s%-22s%-10s\n", "Destination-Server-ID", "Next-Hop-Server-ID", "Cost-of-Path");
   	for(i=0; i<server_num-1; i++) {
   	                 
    printf("%-22d%-22d%-10u\n",dist_obj[i].dest_id, dist_obj[i].next_hop, dist_obj[i].cost);
   	                 
   	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Read topology file and store data in sevrer and DV tables.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void readFromFile(char *filename) {
    
    
    char buf[500]; 
	//char *line;
	
	FILE *fp = fopen(filename, "r");
	if(fp == NULL)
    {
        printf("ERROR: File %s not found.\n", filename);
		exit(1);
	}
    bzero(buf, sizeof buf);
    
    char *line = NULL;
    //char servernum[50],edgenum[50];
    size_t len = 0;
    ssize_t read;
    int i=0,j=0;
    
if ((read = getline(&line, &len, fp)) != -1) {
        
        ///printf("\nRetrieved line of length %zu :", read);
        //printf("%s", line);
        //strcpy(servernum,line);
        server_num=atoi(line);
        //printf("%d", server_num);
        bzero(line, sizeof line);
        }
    
    if ((read = getline(&line, &len, fp)) != -1) {
        
        //printf("\nRetrieved line of length %zu :", read);
        //printf("%s", line);
        //strcpy(edgenum,line);
        neigh_num=atoi(line);
        ///printf("%d", neigh_num);
        bzero(line, sizeof line);
        }
    
    while ((read = getline(&line, &len, fp)) != -1) {
        //printf("\nRetrieved line of length %zu :", read);
        ///printf("%s", line);
        
        //TOKENIZING LOGIC
        char *argument;
        int k=0,portnum=0;
        argument = (char *) malloc(strlen(line)*sizeof(char));
        argument = strtok (line," ");
            while (argument != NULL)
            {
 	            if(k==0)
 	            server_obj[i].server_id= atoi(argument);
 	            if(k==1)
 	            strcpy(server_obj[i].server_ip,argument);
 	            if(k==2) {
 	            portnum=atoi(argument);
 	            sprintf( server_obj[i].server_port, "%d", portnum ); 
 	            }
                argument = strtok (NULL, " ");
	            k++;
            }
            ///printf("From struct id: %d",server_obj[i].server_id);
            ///printf("From struct ip: %s",server_obj[i].server_ip);
            //printf("From struct port: %s",server_obj[i].server_port);
        i++;
        if(i==server_num)
        break;
        //strcpy(servernum,line);
        bzero(line, sizeof line);
        
        }
        
    getMyIP();
    
    while ((read = getline(&line, &len, fp)) != -1 && j<neigh_num) {
        ///printf("\nRetrieved line of length %zu :", read);
        ///printf("%s", line);
        
        //TOKENIZING LOGIC
        char *argument;
        int k=0;
        argument = (char *) malloc(strlen(line)*sizeof(char));
        argument = strtok (line," ");
            while (argument != NULL)
            {
 	            if(k==0)
 	            dist_file_obj[j].host_id= atoi(argument);
 	            if(k==1)
 	            dist_file_obj[j].dest_id= atoi(argument);
 	            if(k==2)
 	            dist_file_obj[j].cost= atoi(argument);
 	            argument = strtok (NULL, " ");
	            k++;
            }
            ///printf("From struct host: %d",dist_file_obj[j].host_id);
            ///printf("From struct dest: %d",dist_file_obj[j].dest_id);	
           /// printf("From struct cost: %d",dist_file_obj[j].cost);
        
        dist_file_obj[j].next_hop=dist_file_obj[j].dest_id;
        neigh_obj[j].dest_id=dist_file_obj[j].dest_id;
        j++;
        //strcpy(servernum,line);
        bzero(line, sizeof line);
        }
   free(line);
   fclose(fp);
   
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get active number of neighbors at given point.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int activeNeigh() {
    int i=0,count=0;
    for(i=0;i<server_num-1;i++) {
        if(dist_obj[i].cost!=65535)
        count++;
    }
    return count;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//initiaize UDP socket for listening to DVs from neighbors.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int initiaizeUDPsocket() {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
	int rv;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	
	//FOR TIME BEING.......................................................................
    //printf("My PORT: %s\n",myPort);
	if ((rv = getaddrinfo(NULL, myPort, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("socket creation failed.\n");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("bind failed\n");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "Failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);
    
    return sockfd;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Send the DV to neighbors.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void sendToNeighbours(char *buf) {
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv,i=0,j=0,k=0;
	int numbytes;
    ///printf("in fun buf:%s\n",buf);
    
    for(i=0;i<neigh_num;i++) {
    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
    
    for(j=0;j<server_num;j++) {
        if(neigh_obj[i].dest_id==server_obj[j].server_id)
        break;
    }
    
    for(k=0;k<server_num-1;k++) {
        if(neigh_obj[i].dest_id==dist_file_obj[k].dest_id)
        break;
    }
    if(dist_file_obj[k].cost==65535)
    continue;
    
	if ((rv = getaddrinfo(server_obj[j].server_ip, server_obj[j].server_port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		error_msg("Couldnot get address\n");
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		error_msg("failed to create socket\n");
	}

	if ((numbytes = sendto(sockfd, buf, strlen(buf), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	///printf("Sent %d bytes to %s\n", numbytes, server_obj[j].server_port);
	close(sockfd);
    }	
	
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Pack DV o host into char string.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char *packMsg() {
    char *buf;
    buf="TESTING IN FUNCTION";
    //printf("in pack:%s\n",buf);
    int i,j;
    
    buf = (char *)malloc(500);
    
    sprintf(buf,"%d#%s#%s#",activeNeigh(),myPort,myIPaddr);
    for(i=0;i<server_num-1;i++) {
            for(j=0;j<server_num;j++) {
                if(dist_obj[i].dest_id==server_obj[j].server_id)
                break;
            }
        if(dist_obj[i].cost!=65535)
        sprintf(buf+strlen(buf),"%s#%s#%d#%u#", server_obj[j].server_ip,server_obj[j].server_port,server_obj[j].server_id,dist_obj[i].cost);
    }
    ///printf("packet is %u bytes\n", strlen(buf));
    ///printf("Packing: %s\n", buf);
    //buf=buffer;
    return (char *)buf;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Unpack the DV received from neighbor and store it in buffer.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int unpackMsg(char *buf ) {
    temp_obj[0]=dist_obj[0];
    temp_obj[1]=dist_obj[1];
    temp_obj[2]=dist_obj[2];
    int i=0,j=0,k=0,m=0;

    
    char *argument;
    char tempIP[LENGTH],tempPort[LENGTH];
    int tempId;
    argument = strtok (buf,"#");
//        while (argument != NULL)
//        {
 	recvNum=atoi(argument);
    
	        //j++;
//        }
    argument = strtok (NULL, "#");    
 	strcpy(recvPort, argument);
    
	    //lastChar = strlen(recvPort- 1);
        //arg[j -1][lastChar] = '\0';
    argument = strtok (NULL, "#");
    strcpy(recvIPaddr, argument);
    
    for(k=0;k<server_num;k++) {
        if(!strcmp(server_obj[k].server_port,recvPort)) 
            recvId=server_obj[k].server_id;
    }
    for(m=0;m<server_num-1;m++) {
        if(dist_obj[m].dest_id==recvId) {
            if(dist_obj[m].cost==65535) {
                ///printf("premature return.\n");
                return 0;
            }
        }
    }
    
        printf("RECEIVED A MESSAGE FROM SERVER:%d\n", recvId);
        for(i=0;i<recvNum;i++) {
            bzero(tempIP, sizeof tempIP);
            bzero(tempPort, sizeof tempPort);
            argument = strtok (NULL, "#");
            strcpy(tempIP, argument);
            
            argument = strtok (NULL, "#");
            strcpy(tempPort, argument);
            
            argument = strtok (NULL, "#");
            tempId=atoi(argument);

            temp_obj[i].host_id=recvId;
            
            for(j=0;j<server_num;j++) {
                if(!strcmp(server_obj[j].server_port,tempPort)) 
                break;
            }
            temp_obj[i].dest_id=server_obj[j].server_id;
            argument = strtok (NULL, "#");
            temp_obj[i].cost=atoi(argument);

            ///printf("%s#%s#%d#%u#\n",tempIP,tempPort,tempId,temp_obj[i].cost);
        }
    
    sprintf( updateCounter[recvId], "%d", 0);
    ///printf("%-22s%-22s%-10s\n", "Destination-Server-ID", "Next-Hop-Server-ID", "Cost-of-Path");
   	///for(i=0; i<recvNum; i++)
        ///printf("%-22d%-22d%-10d\n",temp_obj[i].host_id, temp_obj[i].dest_id, temp_obj[i].cost);
    
    return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Assign the new link cost provided by user
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void updateLinkCost(int host, int dest, unsigned int cost) {
    int i=0,j=0,k=0;
    unsigned int oldValue=0;
    ///printf("host:%d,dest:%d,cost:%u\n",host,dest,cost);
    for(i=0;i<server_num-1;i++) {
        if(host==dist_obj[i].host_id)
            if(dest==dist_obj[i].dest_id) {
                oldValue=dist_obj[i].cost;
                dist_obj[i].cost=cost;
                dist_obj[i].next_hop=dist_obj[i].dest_id;
            }
    }
    for(j=0;j<server_num-1;j++) {
        if(dest==dist_obj[j].next_hop) {
 /*           if(cost==inf) {
                dist_obj[j].cost=inf;
                dist_obj[j].next_hop=dist_obj[j].dest_id;
            }
            else if(cost<oldValue) {
                dist_obj[j].cost-=(oldValue-cost);
            }
            else if(cost>oldValue) {
                dist_obj[j].cost+=(cost-oldValue);
            }
*/          if(dist_obj[j].dest_id!=dest) {  
                for(k=0;k<server_num-1;k++) {
                    if(dist_obj[j].dest_id==dist_file_obj[k].dest_id) {
                        
                            dist_obj[j].cost=dist_file_obj[k].cost;
                            dist_obj[j].next_hop=dist_obj[j].dest_id;
                        
                        ///printf("Assigned old values.\n");
                    }
                }
            }
        }
    }

    if(cost==inf) {
        char buffer[LENGTH];
            for(j=0;j<server_num;j++) {
                if(dest==server_obj[j].server_id)
                break;
            }
        
        sprintf(buffer,"%d#%s#%s#%s#%s#%d#%u#", 1,myPort,myIPaddr,server_obj[j].server_ip,server_obj[j].server_port,server_obj[j].server_id,cost);
        sendToNeighbours(buffer);
        }
    
    ///displayDistTable();
    
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF BELLMAN FORD ALGORITHM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void calculateMinPath() {
    int i,j,k,m;
    ///printf("in calculateMinPath\n");
    for (i=0; i<server_num-1; i++) {
        for(j=0;j<recvNum;j++) {
            if(dist_obj[i].dest_id==temp_obj[j].dest_id) {
                for(k=0;k<neigh_num;k++) {
                    if(temp_obj[j].host_id==dist_obj[k].dest_id)
                    break;
                }
                if((dist_obj[k].cost+temp_obj[j].cost)<dist_obj[i].cost){
                    dist_obj[i].cost=dist_obj[k].cost+temp_obj[j].cost;
                    dist_obj[i].next_hop=dist_obj[k].dest_id;
                    ///printf("UPDATED COST TO %-12d%-12d%-10d\n",dist_obj[i].host_id, dist_obj[i].dest_id, dist_obj[i].cost);
                }
                if((dist_obj[k].cost+temp_obj[j].cost)>dist_obj[i].cost){
                    if(dist_obj[i].next_hop==temp_obj[j].host_id) {
    	                for(m=0;m<server_num-1;m++) {
                            if(dist_obj[j].dest_id==dist_file_obj[m].dest_id) {
                        
                            dist_obj[j].cost=dist_file_obj[m].cost;
                            dist_obj[j].next_hop=dist_obj[j].dest_id;
                        
                            ///printf("Assigned old values.\n");
                            ///printf("UPDATED COST TO %-12d%-12d%-10d\n",dist_obj[i].host_id, dist_obj[i].dest_id, dist_obj[i].cost);
                            }
                        }
                    }
                }
            }
            
            if((dist_obj[i].dest_id==temp_obj[j].host_id) && (dist_obj[i].host_id==temp_obj[j].dest_id)) {
                if(dist_obj[i].cost!=temp_obj[j].cost) {
                    updateLinkCost(dist_obj[i].host_id,dist_obj[i].dest_id,temp_obj[j].cost);
                }
            }
            
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Sort DV Table in ascending order.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void sortDistTable() {
    int i=0,j=0;
    struct dist_table temp[5];
    for(i=0;i<server_num-1;i++)
        for(j=i+1;j<server_num-1;j++)
            if(dist_obj[i].dest_id>dist_obj[j].dest_id) {
                temp[0]=dist_obj[i];
                dist_obj[i]=dist_obj[j];
                dist_obj[j]=temp[0];
            }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Disable the link provided by user.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void disableLink(int link) {

    updateLinkCost(myConnId,link,inf);
    int i=0;
    for(i=0;i<server_num-1;i++) {
        if(dist_file_obj[i].dest_id==link)
            dist_file_obj[i].cost=inf;
    }
    //neigh_num--;
        
    
}

int main(int argc, char *argv[]) {

    int success=1;	
	printf("...Welcome...\n");
	if (argc != 5)
	   {
	       error_msg("Please pass proper argumets\n");
	   }

	updateTime=atoi(argv[4]);
	strcpy(filename,argv[2]);

	//printf("ARG: %s interval: %d\n", filename, updateTime);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Read topology file.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    readFromFile(filename);
    initiaizeDistTable();
    updateDistTable();
    
    //displayDistTable();
    
    printf("IP:%s Port:%s\n",myIPaddr,myPort);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Initialize UDP socket
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    int sockfd=initiaizeUDPsocket();
    
    struct timeval tv;
    fd_set readfds;


    char ip[100];
    char arg[100][100];
    int lastChar;
    tv.tv_sec = updateTime;
    tv.tv_usec =0;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    FD_SET(sockfd, &readfds);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Call select function in infinite loop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    for(;;) {
    
        if(success==1) {
        printf("[PA2]$");
        success=0;
        }
    
    fflush(stdout);
    
    if(select(sockfd+1, &readfds, NULL, NULL, &tv) == -1) {
        error_msg("Error in select function\n");
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Take user input
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (FD_ISSET(0, &readfds)) {
        
        int j=0;
        char buffer[256];
            
        fflush(stdout);
            
        char *argument;
        fgets(ip,100,stdin);
        argument = (char *) malloc(strlen(ip)*sizeof(char));
        fflush(stdout);
            
        argument = strtok (ip," ");
        while (argument != NULL)
        {
 	        fflush(stdout);
 	        strcpy(arg[j], argument);
 	        //arg[j]=argument;
            argument = strtok (NULL, " ");
	        j++;
        }
        lastChar = strlen(arg[j - 1]) - 1;
        arg[j -1][lastChar] = '\0';
	       
        if(!strcasecmp(arg[0],"display"))
        if (j>1) {
	        printf("DISPLAY: INCORRECT ARGUMENTS.\n");
	        success=1;
        }
   	    else
        {
            
            sortDistTable();
            displayDistTable();
            printf("DISPLAY: SUCCESS\n");
            success=1;
        }

        else if(!strcasecmp(arg[0],"packets"))
        if (j>1) {
	        printf("PACKETS: INCORRECT ARGUMENTS.\n");
	        success=1;
        }
   	    else
        {
            printf("Number of packets received since last invocation:%d\n",packet_counter);
            printf("PACKETS: SUCCESS\n");
            packet_counter=0;
            success=1;
        }
        
        else if(!strcasecmp(arg[0],"step"))
        if (j>1) {
	        printf("STEP: INCORRECT ARGUMENTS.\n");
	        success=1;
        }
   	    else 
        {
            if(activeNeigh()==0)
                printf("STEP: Don't have any neighbours.\n");
            else {
                char buffer[LENGTH];
                //strcpy(buffer,"TEST MESSAGE");
                bzero(buffer, sizeof buffer);
                strcpy(buffer,packMsg());
                //printf("SENDING buf:%s\n",buffer);
                sendToNeighbours(buffer);
                printf("STEP: SUCCESS\n");
                
            }
            
            success=1;
        }
        
        else if(!strcasecmp(arg[0],"update"))
        if (j!=4) {
	        printf("UPDATE: INCORRECT ARGUMENTS.\n");
	        success=1;
        }
   	    else
        {
            ///printf("Received update\n");
            if(!strcasecmp(arg[3],"inf"))
            strcpy(arg[3],"65535");
            updateLinkCost(atoi(arg[1]),atoi(arg[2]),atoi(arg[3]));
                char buffer[LENGTH];
                //strcpy(buffer,"TEST MESSAGE");
                bzero(buffer, sizeof buffer);
                strcpy(buffer,packMsg());
                ///printf("SENDING buf:%s\n",buffer);
                sendToNeighbours(buffer);
            printf("UPDATE: SUCCESS\n");
            sortDistTable();
            displayDistTable();
            success=1;
        }

        else if(!strcasecmp(arg[0],"disable"))
        if (j!=2) {
	        printf("DISABLE: INCORRECT ARGUMENTS.\n");
	        success=1;
        }
   	    else
        {
            ///printf("Received disable\n");
            int m=0;
            for(m=0;m<neigh_num;m++) {
                if(neigh_obj[m].dest_id==atoi(arg[1]))
                break;
            }
            if(m==neigh_num) {
                printf("DISABLE: %s NOT MY NEIGHBOR.\n",arg[1]);
            }
            else {
            disableLink(atoi(arg[1]));
            //sortDistTable();
            //displayDistTable();
            printf("DISABLE: SUCCESS\n");
            }
            success=1;
        }

        else if(!strcasecmp(arg[0],"crash"))
        if (j!=1) {
	        printf("CRASH: INCORRECT ARGUMENTS.\n");
	        success=1;
        }
   	    else
        {
            ///printf("Received crash\n");
            initiaizeDistTable();
            printf("CRASH: SUCCESS\n");
            success=1;
        }
        
        else {
            printf("Wrong command entered.\n");
            success=1;
            }
        FD_ZERO(&readfds);
        FD_SET(0, &readfds);
        FD_SET(sockfd, &readfds);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Process received DV
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (FD_ISSET(sockfd, &readfds)) {

        struct sockaddr_storage their_addr;
	    char buf[MAXBUFLEN];
	    socklen_t addr_len;
	    int numbytes;
	    char *buffer;
	    addr_len = sizeof their_addr;
	    char s[INET6_ADDRSTRLEN];

	    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
		    (struct sockaddr *)&their_addr, &addr_len)) == -1) {
		error_msg("Error in recvfrom.");
		
	    }

	    /*printf("RECEIVED A MESSAGE FROM SERVER %s\n",
		    inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s)); */
	    ///printf("Packet is %d bytes long\n", numbytes);
	    buf[numbytes] = '\0';
	    ///printf("Packet contains \"%s\"\n", buf);

        buffer=(char *) malloc(strlen(buf)*sizeof(char));
	    buffer=buf;
	    if(unpackMsg(buffer)) {
//Block infinite msg	    if(recvId==)
	        packet_counter++;
	        calculateMinPath();
	    }
	    FD_ZERO(&readfds);
        FD_SET(0, &readfds);
        FD_SET(sockfd, &readfds);
	    success=1;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Server has timed out,send periodic DV to neighbors
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else {
    ///printf("Timed out.\n");

    if(activeNeigh()==0)
        printf("Don't have any neighbours.\n");
    else {
        char buffer[LENGTH];
        //strcpy(buffer,"TEST MESSAGE");
        bzero(buffer, sizeof buffer);
        strcpy(buffer,packMsg());
        ///printf("SENDING buf:%s\n",buffer);
        sendToNeighbours(buffer);
    }

    int a=0,b=0,c=0;;
    for(a=0;a<server_num+1;a++) {
        sprintf( updateCounter[a], "%d", (atoi(updateCounter[a])+1));
        if(atoi(updateCounter[a])>3) {
            for(b=0;b<neigh_num;b++) {
                if(a==neigh_obj[b].dest_id) {
                    for(c=0;c<server_num-1;c++) {
                        if(a==dist_obj[c].dest_id) {
                            if(dist_obj[c].cost!=inf) {
                            ///printf("neighbour %d seems unresponsive.\n",a);
                            disableLink(a);
                            }
                        }
                    }
                }
            }
        }
    }
    
    tv.tv_sec = updateTime;
    tv.tv_usec =0;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    FD_SET(sockfd, &readfds);

    success=1;
    }
}
return 0;
}

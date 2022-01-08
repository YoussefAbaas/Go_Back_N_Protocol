#include <bits/stdc++.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"
using namespace std;
/***************************************************************/
bool network_layer_state=true;
unsigned int client;
unsigned int server;
timer t;
packet network_packets[8];
int network_counter=0;
event_type current_event=network_layer_ready;
/****************************************************************/
void Connect_Master();
void Connect_Slave();
/****************************************************************/
static bool between(seq_nr a, seq_nr b, seq_nr c)
{
/* Return true if a <= b < c circularly; false otherwise. */
if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
return true;
else
return false;
}
static void send_data(seq_nr frame_nr, seq_nr frame_expected, packet buffer[ ])
{
/* Construct and send a data frame. */
frame s; /* scratch variable */
s.info = buffer[frame_nr]; /* insert packet into frame */
s.seq = frame_nr; /* insert sequence number into frame */
s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1); /* piggyback ack */
to_physical_layer(&s); /* transmit the frame */
start_timer(frame_nr); /* start the timer running */
printf("frame sent to physical layer: %d\n", s.info.data[0]);
}
/****************************************************************/
int main()
{
char ans;
int state;
int missed_ack;
cout<<"Do you want to miss acknowledge? [Y/N]"<<endl;
cin>>ans;
if(ans=='Y')
{
    state=1;
    cout<<"enter number of missed frame"<<endl;
    cin>>missed_ack;
}   
else 
{
    state=0;
}     
int next_seq_num=0; // frame num to be received
frame r; // received frame
frame s; // sent frame (ack)
Connect_Slave();
int flag=0; // flag for missed ack
while(true)
{
from_physical_layer(&r);
if(r.seq==missed_ack&state==1)flag++;
if(r.seq==next_seq_num&&flag!=1)
{
    s.ack=next_seq_num;
    to_physical_layer(&s);
    inc(next_seq_num);
}
else //not correct frame
{
s.ack=next_seq_num-1;
to_physical_layer(&s);
}
}
}
/*************************************************************************/
void enable_network_layer(void)
{
    network_layer_state=true;
}
void disable_network_layer(void)
{
    network_layer_state=false;
}
void to_physical_layer(frame*s){
    write(client, s, sizeof(frame));
}
void from_physical_layer(frame*r){
    read(client, r, sizeof(frame));
}
void start_timer(seq_nr k)
{
t.flag[k]=true;
}
void stop_timer(seq_nr k)
{
t.flag[k]=false;
}
void from_network_layer(packet*p)
{
    *p=network_packets[(network_counter++)%8];
}
void to_network_layer(packet *p)
{

}
void wait_for_event(event_type *event)
{
    *event=current_event;
}
void Connect_Slave(void)
{
    client = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    printf("Waiting for Connection...\n");
    while (connect(client, (struct sockaddr *)&address, sizeof(address)));
    printf("Slave Connected\n");
}

void Connect_Master(void){
            printf("Master Waiting\n");
            server = socket(AF_INET, SOCK_STREAM, 0);
            int opt=1;
            setsockopt(server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt));
            struct sockaddr_in address;
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = inet_addr("127.0.0.1");
            address.sin_port = htons(PORT);
            bind(server, (struct sockaddr *)&address, sizeof(address));
            listen(server, 1);
            client = accept(server, NULL, NULL);
            printf("Slave connected\n");
}

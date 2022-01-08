#include <bits/stdc++.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"
using namespace std;
/*****************************************************************/
bool network_layer_state=true;
unsigned int client;
unsigned int server;
timer t;
packet network_packets[8];
int network_counter=0;
event_type current_event=network_layer_ready;
int flag=0; //states flag
int timeout_flag=0; // time out flag
/****************************************************************/
void Connect_Slave(void);
void Connect_Master(void);
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
    network_packets[0].data[0]=0;
    network_packets[1].data[0]=1;
    network_packets[2].data[0]=2;
    network_packets[3].data[0]=3;
    network_packets[4].data[0]=4;
    network_packets[5].data[0]=5;
    network_packets[6].data[0]=6;
    network_packets[7].data[0]=7;
seq_nr next_frame_to_send; /* MAX SEQ > 1; used for outbound stream */
seq_nr ack_expected=0; /* oldest frame as yet unacknowledged */
seq_nr frame_expected; /* next frame expected on inbound stream */
frame r; /* scratch variable */
packet buffer[MAX_SEQ + 1]; /* buffers for the outbound stream */
seq_nr nbuffered; /* number of output buffers currently in use */
seq_nr i; /* used to index into the buffer array */
event_type event;
Connect_Master();
enable_network_layer(); /* allow network layer ready events */
ack_expected = 0; /* next ack expected inbound */
next_frame_to_send = 0; /* next frame going out */
frame_expected = 0; /* number of frame expected inbound */
nbuffered = 0; /* initially no packets are buffered */
while (true) {
wait_for_event(&event); /* four possibilities: see event type above */
switch(event) {
case network_layer_ready: /* the network layer has a packet to send */
/* Accept, save, and transmit a new frame. */
cout<<"Fetch new packet from network layer "<<endl;
from_network_layer(&buffer[next_frame_to_send]); /* fetch new packet */
nbuffered = nbuffered + 1; /* expand the sender’s window */
send_data(next_frame_to_send, frame_expected, buffer);/* transmit the frame */
inc(next_frame_to_send); /* advance sender’s upper window edge */
break;
case frame_arrival: /* a data or control frame has arrived */
from_physical_layer(&r); /* get incoming frame from physical layer */
if (r.ack == frame_expected) {
cout<<"Frame arrival from physical layer"<<endl;
printf("frame sent from physical layer is ack for frame: %d\n", r.ack);
/* Frames are accepted only in order. */
to_network_layer(&r.info); /* pass packet to network layer */
inc(frame_expected); /* advance lower edge of receiver’s window */
}
/* Ack n implies n − 1, n − 2, etc. Check for this. */
while (between(ack_expected, r.ack, next_frame_to_send)) {
/* Handle piggybacked ack. */
nbuffered = nbuffered - 1; /* one frame fewer buffered */
stop_timer(ack_expected); /* frame arrived intact; stop timer */
inc(ack_expected); /* contract sender’s window */
}
break;
case cksum_err:
cout<<"Check Sum error"<<endl;
break; /* just ignore bad frames */
case timeout: /* trouble; retransmit all outstanding frames */
cout<<"Timerout"<<endl;
next_frame_to_send = ack_expected; /* start retransmitting from last message did not receive ack */
for (i = 1; i <= nbuffered; i++) {
send_data(next_frame_to_send, frame_expected, buffer);/* resend frame */
inc(next_frame_to_send); /* prepare to send the next one */
}
current_event=frame_arrival;
}
if (nbuffered < MAX_SEQ)
enable_network_layer();
else
disable_network_layer();
/******************************Handling States************************/

/*frame arrival*/
if(flag==1)
{
    current_event=frame_arrival;
    flag=0;
}
else{
current_event=network_layer_ready;
flag=1;
}
//check time out 
  for(int i=0;i<MAX_SEQ;i++){
            if(t.flag[i]==true){
                t.count[i]++;
                if (t.count[i]==10&&timeout_flag==0) {
                    timeout_flag=1;
                    cout<<"time out for frame "<<i<<endl;
                    current_event=timeout;
                    flag=1;break;
                    }
            }
            
        }
        for(int i=0;i<1000000000;i++);
}

}
/*****************************************************************/
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
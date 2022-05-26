/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/19
 */
#include "common.h"
#include <sys/wait.h>
#include "FIFOreqchannel.h"
#include "MQreqchannel.h"
#include "SHMreqchannel.h"
#include "Reqchannel.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;


int main(int argc, char *argv[]){
    
    int c;
    int buffercap = MAX_MESSAGE;
    int p = 0, ecg = 1;
    double t = -1.0;
    bool isnewchan = false;
    bool isfiletransfer = false;
    string filename;
    string ipc_method = "f";
    int nchannels = 1;


    while ((c = getopt (argc, argv, "p:t:e:m:f:c:i:")) != -1){
        switch (c){
            case 'p':
                p = atoi (optarg);
                break;
            case 't':
                t = atof (optarg);
                break;
            case 'e':
                ecg = atoi (optarg);
                break;
            case 'm':
                buffercap = atoi (optarg);
                break;
            case 'c':
                isnewchan = true;
                nchannels = atoi(optarg);
                break;
            case 'f':
                isfiletransfer = true;
                filename = optarg;
                break;
            case 'i':
                ipc_method = optarg;
                break;
        }
    }
    
    // fork part
    if (fork()==0){ // child 
	
		char* args [] = {"./server", "-m", (char *) to_string(buffercap).c_str(), "-i", (char *) ipc_method.c_str(), NULL};
        if (execvp (args [0], args) < 0){
            perror ("exec filed");
            exit (0);
        }
    }

    vector<RequestChannel*> channels_vec; // to store all the channels for execution later
    RequestChannel* control_chan = NULL; // init
    RequestChannel* chan = control_chan; // init

    //cout << "num channels: " << nchannels << endl;

    for(int i = 0; i < nchannels; i++){ // makes channels according to num of channels

        control_chan = NULL;
        if(ipc_method == "f"){
            control_chan = new FIFORequestChannel ("control", RequestChannel::CLIENT_SIDE);
        }
        else if(ipc_method == "q"){
            control_chan = new MQRequestChannel ("control", RequestChannel::CLIENT_SIDE);
        }
        else if(ipc_method == "s"){
            control_chan = new SHMRequestChannel ("control", RequestChannel::CLIENT_SIDE, buffercap);
        }
        
        chan = control_chan;
        if (isnewchan){
            cout << "Using the new channel everything following" << endl;
            MESSAGE_TYPE m = NEWCHANNEL_MSG;
            control_chan->cwrite (&m, sizeof (m));
            char newchanname [100];
            control_chan->cread (newchanname, sizeof (newchanname));
            if(ipc_method == "f"){
                chan = new FIFORequestChannel (newchanname, RequestChannel::CLIENT_SIDE);
            }
            else if(ipc_method == "q"){
                chan = new MQRequestChannel (newchanname, RequestChannel::CLIENT_SIDE);
            }
            else if(ipc_method == "s"){
                chan = new SHMRequestChannel (newchanname, RequestChannel::CLIENT_SIDE, buffercap);
            }
            
            cout << "New channel by the name " << newchanname << " is created" << endl;
            cout << "All further communication will happen through it instead of the main channel" << endl;
        }
        channels_vec.push_back(chan);
        
    }


    if (!isfiletransfer){   // requesting data msgs
        for (int i = 0; i< nchannels; i++){
            if (t >= 0){    // 1 data point
                cout << "1 data point" << endl;
                datamsg d (p, t, ecg);
                channels_vec.at(i)->cwrite (&d, sizeof (d));
                double ecgvalue;
                channels_vec.at(i)->cread (&ecgvalue, sizeof (double));
                cout << "Ecg " << ecg << " value for patient "<< p << " at time " << t << " is: " << ecgvalue << endl;
                cout << "chan here" << endl;

            }
            else{          // bulk (i.e., 1K) data requests, put into file
                //cout << "here" << endl;
                double ts = 0;  
                datamsg d (p, ts, ecg);
                double ecgvalue;

                stringstream ss;
                ss << i;
                
                ofstream ofs;
                string outfile = "received/chan" + ss.str() + ".txt";
                ofs.open(outfile);

                high_resolution_clock::time_point start_time = high_resolution_clock::now();

                for (int j=0; j<1000; j++){
                    //cout << "hmmmm" << endl;
                    channels_vec.at(i)->cwrite (&d, sizeof (d));
                    channels_vec.at(i)->cread (&ecgvalue, sizeof (double));
                    d.seconds += 0.004; //increment the timestamp by 4ms
                    //cout << ecgvalue << endl;
                    ofs << ecgvalue << endl;
                }
                //fs.close();
                high_resolution_clock::time_point end_time = high_resolution_clock::now();
                double duration = duration_cast<microseconds>(end_time - start_time).count();
                cout << "the duration to collect 1K data points is: " << duration/1000000 <<" seconds" << endl;
                //cout << "here" << endl;
            }
        }
    }
    else if (isfiletransfer){
        // part 2 requesting a file
        filemsg f (0,0);  // special first message to get file size
        int to_alloc = sizeof (filemsg) + filename.size() + 1; // extra byte for NULL
        char* buf = new char [to_alloc];
        memcpy (buf, &f, sizeof(filemsg));
        strcpy (buf + sizeof (filemsg), filename.c_str());
        chan->cwrite (buf, to_alloc);
        __int64_t filesize;
        chan->cread (&filesize, sizeof (__int64_t));
        cout << "File size: " << filesize << endl;

        //int transfers = ceil (1.0 * filesize / MAX_MESSAGE);
        filemsg* fm = (filemsg*) buf;
        //__int64_t rem = filesize;
        string outfilepath = string("received/") + filename;
        FILE* outfile = fopen (outfilepath.c_str(), "wb");  
        fm->offset = 0;
        char* recv_buffer = new char [MAX_MESSAGE];

        for (int p = 0; p < nchannels; p++){
            __int64_t rem = filesize/nchannels;

            high_resolution_clock::time_point start_time_2 = high_resolution_clock::now();

            while (rem>0){
                fm->length = (int) min (rem, (__int64_t) MAX_MESSAGE);
                channels_vec.at(p)->cwrite (buf, to_alloc);
                channels_vec.at(p)->cread (recv_buffer, MAX_MESSAGE);
                fwrite (recv_buffer, 1, fm->length, outfile);
                rem -= fm->length;
                fm->offset += fm->length;
            }
            high_resolution_clock::time_point end_time_2 = high_resolution_clock::now();
            double duration2 = duration_cast<microseconds>(end_time_2 - start_time_2).count();
            cout << "the duration for file transfers is: " << duration2/1000000 << " seconds" << endl;
        }
        fclose (outfile);
        delete recv_buffer;
        delete buf;
        cout << "File transfer completed" << endl;
    }
    //close(outfilename);
    
    MESSAGE_TYPE q = QUIT_MSG;
    for(int k = 0; k < nchannels; k++){
        channels_vec.at(k)->cwrite (&q, sizeof (MESSAGE_TYPE));
        delete channels_vec.at(k);
    }
    if (chan != control_chan){ // this means that the user requested a new channel, so the control_channel must be destroyed as well 
        control_chan->cwrite (&q, sizeof (MESSAGE_TYPE));
        delete control_chan;
    }
	// wait for the child process running server
    // this will allow the server to properly do clean up
    // if wait is not used, the server may sometimes crash
	wait (0);
    
}

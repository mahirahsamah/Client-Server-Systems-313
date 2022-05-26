/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
	Original author of the starter code
	
	Please include your name and UIN below
	Name: Mahirah Samah
	UIN: 828007237
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <fstream>
#include <cmath>
#include <chrono>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>

using namespace std;
using namespace std::chrono;
using std::string;

int main(int argc, char *argv[]){

	// for truncate part: truncate -s 100MB empty100.bin
	// put this file into BIMDC manually
	// then run ./client -f empty100.bin
	// second part: then run diff BIMDC/empty100.bin received/empty100.bin

    FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
	
	int opt;
	//int c;
	int p = 1;
	double t = 0.0;
	int e = 1;
	bool new_chan_exists = false;
	int maxmsg = MAX_MESSAGE;
	
	string filename;

	while ((opt = getopt(argc, argv, "p:t:e:f:c:m")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'c':
				new_chan_exists = true;
				break;
			case 'm':
				maxmsg = atoi(optarg);
				break;
		}
	}

	//server as child process fork

	
	/*pid_t process;
	process = fork();
	if (process == 0) {
		cout << " * Tesing fork() using a new channel * " << endl;
		char *args[] = {"./server", "-m", (char *) to_string(maxmsg).c_str(), NULL};
		if (execvp (args [0], args) < 0) {
			perror ("exec filed");
			exit(0);
		}
	}*/

	// creating new channel: WORKS WHEN chan. in "requesting data msgs" is set to new_chan->
	/*
	FIFORequestChannel* new_channel = new FIFORequestChannel ("control", FIFORequestChannel::CLIENT_SIDE);
	FIFORequestChannel* new_chan = new_channel;
	//FIFORequestChannel new_chan ("control", FIFORequestChannel::CLIENT_SIDE);
	if (new_chan_exists) {
		cout << "creating new channel..." << endl;
		MESSAGE_TYPE chanmsg = NEWCHANNEL_MSG;
		new_channel->cwrite(&chanmsg, sizeof(chanmsg));
		char chan_buf[MAX_MESSAGE];
		new_channel->cread(chan_buf, sizeof(chan_buf));
		new_chan = new FIFORequestChannel(chan_buf, FIFORequestChannel::CLIENT_SIDE);
		cout << "new channel created!" << endl;
	}*/

	// DATA MESSAGE - REQUESTING DATA POINTS

	
	ofstream fout;
	fout.open("x1.csv");
	
    // sending a non-sense message, you need to change this
    //char buf [MAX_MESSAGE]; // 256
	/*datamsg d (p=1, t=59.00, e=1);
	
	chan.cwrite (&d, sizeof (datamsg));
	double reply;
	int nbytes = chan.cread(&reply, sizeof(double));*/

	
	// requesting 3 data points for testing the new channel*/
	
	// data point 1
	/*
	high_resolution_clock::time_point onedata_start_time = high_resolution_clock::now();

    datamsg d1 (p=1, t=59.00, e=1);
	
	chan.cwrite (&d1, sizeof (datamsg));  // question

	double reply1;
	int nbytes1 = chan.cread(&reply1, sizeof(double)); //answer
	high_resolution_clock::time_point onedata_end_time = high_resolution_clock::now();
	cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply1 << endl;
	auto onedata_duration = (onedata_end_time - onedata_start_time).count();
	cout << "one data request duration was: " << onedata_duration << endl;
	*/
	// data point 2
	/*
	datamsg d2 (p=3, t=59.00, e=1);
	
	new_chan->cwrite (&d2, sizeof (datamsg));  // question

	double reply2;
	int nbytes2 = new_chan->cread(&reply2, sizeof(double)); //answer

	cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply2 << endl;

	// data point 3

	datamsg d3 (p=9, t=59.00, e=1);
	
	new_chan->cwrite (&d3, sizeof (datamsg));  // question

	double reply3;
	int nbytes3 = new_chan->cread(&reply3, sizeof(double)); //answer

	cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply3<< endl;*/

	
	// for loop here
	//double t = 0;
	//if(t == 0) {
		
		high_resolution_clock::time_point manydata_start_time = high_resolution_clock::now();
		for (int i = 0; i < 1000; i++) {
			t += 0.004;
			char buf [MAX_MESSAGE];

			double reply1;
			double reply2;

			e=2;
			datamsg y (p, t, e);
			chan.cwrite (&y, sizeof (datamsg)); // question
			int nbytes = chan.cread (&reply2, sizeof(double)); //answer
			cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " <<  reply2 << endl;

			e=1;
			datamsg x (p, t, e);
			chan.cwrite (&x, sizeof (datamsg)); // question
			nbytes = chan.cread (&reply1, sizeof(double)); //answer
			cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " <<  reply1 << endl;

			fout << t <<"\t"<< reply1 << "\t" <<reply2 << endl;
		}
		high_resolution_clock::time_point manydata_end_time = high_resolution_clock::now();
		
		auto manydata_duration = (manydata_end_time - manydata_start_time).count();
		cout << "many data requests duration was: " << manydata_duration << endl;
	//}*/

	/*
	else if(t > 0) {
		datamsg x (p, t, e=1);
	
		chan.cwrite (&x, sizeof (datamsg)); // question

		double reply;
		int nbytes = chan.cread (&reply, sizeof(double)); //answer
		cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " <<  reply << endl;
	}
	fout.close();*/
	

	// REQUESTING FILES: WORKS

	/*
	filemsg fm (0,0); // to get its length
	//string fname = filename;

	int len = sizeof (filemsg) + filename.size()+1; // length of file?

	char buf2 [len];
	memcpy (buf2, &fm, sizeof (filemsg));
	strcpy (buf2 + sizeof (filemsg), filename.c_str());
	chan.cwrite (buf2, len);  // I want the file length;
	
	__int64_t filelength; // contains the length of file
	chan.cread(&filelength, sizeof(__int64_t));
	
	int num_req = filelength/MAX_MESSAGE;
	int buf_size = 0;

	ofstream file_fout;
	string outfilepath = string("received/") + filename;
	file_fout.open(outfilepath);

	high_resolution_clock::time_point file_start_time = high_resolution_clock::now();

	for(int i = 0; i < num_req; i++) {
		if(i != (num_req-1)) {
			filemsg a (buf_size, MAX_MESSAGE);
			memcpy (buf2, &a, sizeof (filemsg));
			strcpy (buf2 + sizeof (filemsg), filename.c_str());
			chan.cwrite (buf2, len);
			char buf_r [MAX_MESSAGE];
			chan.cread(buf_r, MAX_MESSAGE); // receiving buffer
			file_fout << buf_r;
		}
		else{
			int new_buf = (filelength - buf_size); 
			filemsg a (buf_size, new_buf);
			memcpy (buf2, &a, sizeof (filemsg));
			strcpy (buf2 + sizeof (filemsg), filename.c_str());
			chan.cwrite (buf2, len);
			char buf_r [new_buf];
			chan.cread(buf_r, new_buf);
			file_fout << buf_r;
		}
		
		buf_size += MAX_MESSAGE;
		
	}
	high_resolution_clock::time_point file_end_time = high_resolution_clock::now();
	auto file_duration = (file_end_time - file_start_time).count();
	cout << "file transfer duration was: " << file_duration << endl;
	file_fout << endl;
	file_fout.close();*/

	
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite (&m, sizeof (MESSAGE_TYPE)); // was chan

	/*MESSAGE_TYPE quitmsg = QUIT_MSG;
	new_chan->cwrite(&quitmsg, sizeof(MESSAGE_TYPE));*/

	wait(0);
}

#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
#include <chrono>

using namespace std;
using namespace std::chrono;

struct Response{
    int person; // person num
    double ecg;
    Response (int _p, double _e): person(_p), ecg(_e){;}
};

FIFORequestChannel* create_new_channel(FIFORequestChannel* mainchan){
    char name [1024];
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    mainchan->cwrite(&m, sizeof (m));
    mainchan->cread(name, 1024);
    FIFORequestChannel* newchan = new FIFORequestChannel(name, FIFORequestChannel::CLIENT_SIDE);
    return newchan;
}

void patient_thread_function(int n, int pno, BoundedBuffer* request_buffer){
    /* What will the patient threads do? */
    double t = 0.0;
    datamsg d(pno, t, 1);
    //double resp = 0;
    for(int i = 0; i < n; i++){
        request_buffer->push((char *) &d, sizeof(d));
        d.seconds += 0.004;
    }

}

void worker_thread_function(int mb, BoundedBuffer* request_buffer, FIFORequestChannel* wchan, BoundedBuffer* response_buffer){
    char buf[1024];
    double resp = 0;
    char recvbuf[mb];

    while(true){
        request_buffer->pop(buf, 1024);
        MESSAGE_TYPE* m = (MESSAGE_TYPE *) buf;

        if(*m == QUIT_MSG){
            wchan->cwrite(m, sizeof(MESSAGE_TYPE));
            delete wchan;
            break;
        }
        else if(*m == DATA_MSG){
            datamsg* d = (datamsg *) buf;
            wchan->cwrite(d, sizeof(datamsg));
            //double ecg;
            wchan->cread(&resp, sizeof(double));
            Response r(d->person, resp);
            response_buffer->push((char *) &r, sizeof(Response));
        }
        else if(*m == FILE_MSG){
            //cout << "here at work thread func, file_msg" << endl;
            /* //cout << "here" << endl;
            filemsg* fm = (filemsg *) buf;
            //string fname = (char *)(fm + 1); // advance pointer
            string fname = (char *)fm;
            int sz = sizeof(filemsg) + fname.size() + 1;
            wchan->cwrite(buf, sz);
            wchan->cread(recvbuf, mb);

            string recvname = "recv/" + fname;
            FILE* fp = fopen(recvname.c_str(), "r+");
            fseek(fp, fm->offset, SEEK_SET);
            fwrite(recvbuf, 1, fm->length, fp);
            //request_buffer->push(); - break up into chunks
            fclose(fp);
            //cout << "file end" << endl; */

            filemsg *fm = (filemsg *)buf;
            string fileName = (char *)(fm+1);
            int to_alloc = sizeof(filemsg) + fileName.size() + 1; // extra byte for NULL

            string outfilepath = "recv/" + fileName;

            wchan->cwrite(buf, to_alloc);
            wchan->cread(recvbuf, mb); //read output data

            FILE *outfile = fopen(outfilepath.c_str(), "r+"); //specific mode that allows file chunks to be stored in teh correct order

            fseek(outfile, fm->offset, SEEK_SET);
            //fm->offset = 0;
            //char* recv_buffer = new char [MAX_MESSAGE];

            fwrite(recvbuf, 1, fm->length, outfile);
            fclose(outfile);
        
        }
    }
}

void histogram_thread_function (BoundedBuffer* response_buffer, HistogramCollection* hc){
    char buf[1024];
    Response* r = (Response *) buf;
    while(true){
        response_buffer->pop(buf, 1024);
        //Response* r = (Response *) buf;
        if(r->person < 1){
            break;
        }
        //else{
        hc->update(r->person, r->ecg);
    }
}

void file_thread_function(string fname, BoundedBuffer* request_buffer, FIFORequestChannel* chan, int mb){
    // 1. create the file
    //string recvfname = "recv/" + fname;
    
    char buf[1024];
    filemsg f(0, 0);
    memcpy(buf, &f, sizeof(f));
    strcpy(buf+sizeof(f), fname.c_str());
    chan->cwrite(buf, sizeof(f) + fname.size() + 1);
    __int64_t filelength;
    chan->cread(&filelength, sizeof(filelength));

    string recvfname = "recv/" + fname;
    FILE* fp = fopen(recvfname.c_str(), "w");

    fseek(fp, filelength, SEEK_SET);
    fclose(fp);
    // 2. generate all the file messages
    filemsg* fm = (filemsg *) buf;
    __int64_t remlen = filelength;
    while(remlen > 0){
        fm->length = min (remlen, (__int64_t) mb);
        request_buffer->push(buf, sizeof(filemsg) + fname.size() + 1);
        fm->offset += fm->length;
        remlen -= fm->length;
    }
}


int main(int argc, char *argv[])
{
    int c;
    int buffercap = MAX_MESSAGE;
    int n = 100;
    int ecg = 1;
    double t = -1.0;    		//d0efault number of requests per "patient"
    int p = 10;     		// number of patients [1,15]
    int w = 100;    		//default number of worker threads
    int b = 20; 		// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the message buffer
    srand(time_t(NULL));
    bool isfiletransfer = false;
    string fname;
    int h = 3;
    int opt = -1;
    bool isnewchan = false;


    while ((c = getopt (argc, argv, "p:t:e:m:c:f:b:w:n:h:")) != -1){
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
                m = buffercap;
                break;
            case 'c':
                isnewchan = true;
                break;
            case 'f':
                isfiletransfer = true;
                fname = optarg;
                //cout << "ft true" << endl;
                break;
            case 'b':
                b = atoi (optarg);
                break;
            case 'w':
                w = atoi (optarg);
                break;
            case 'n':
                n = atoi (optarg);
                break;
            case 'h':
                h = atoi (optarg);
                break;
        }
    }

    if (fork()==0){ // child 
	
		char* args [] = {"./server", "-m", (char *) to_string(buffercap).c_str(), NULL};
        if (execvp (args [0], args) < 0){
            perror ("exec filed");
            exit (0);
        }
    }
    
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
    BoundedBuffer response_buffer(b);
	HistogramCollection hc;
    
    // making a histogram h and addidng it to the collection hc
	for(int i = 0; i< p; i++){
        Histogram* h = new Histogram(10, -2.0, 2.0);
        hc.add(h);
    }

    FIFORequestChannel* wchans[w];
    for(int i = 0; i < w; i++){
        wchans[i] = create_new_channel(chan);
    }
    
    /* Start all threads here */
    high_resolution_clock::time_point start_time = high_resolution_clock::now();
    
    if(!isfiletransfer){
        thread patients[p];
        for(int i = 0; i < p; i++){
            patients[i] = thread(patient_thread_function, n, i+1, &request_buffer);
        }

        thread workers[w];
        for(int i = 0; i < w; i++){
            workers[i] = thread(worker_thread_function, m, &request_buffer, wchans[i], &response_buffer);

        }

        thread hists[h];
        for(int i = 0; i < h; i++){
            hists[i] = thread(histogram_thread_function, &response_buffer, &hc);

        }

        // joining
        for(int i = 0; i < p; i++){
            patients[i].join();
        }

        for(int i = 0; i<w; i++){
            MESSAGE_TYPE q = QUIT_MSG;
            request_buffer.push((char *) &q, sizeof(q));
        }
        for(int i = 0; i < w; i++){
            workers[i].join();
        }

        for(int i = 0; i < h; i++){
            datamsg d(-1, 0, -1);
            response_buffer.push((char *)&d, sizeof(d));
        }
        for(int i = 0; i < h; i++){
            hists[i].join();
        }
        hc.print ();
    }

    /*thread filethread(file_thread_function, fname, chan, m, &request_buffer);

    thread workers[w];
    for(int i = 0; i < w; i++){
        workers[i] = thread(worker_thread_function, m, &request_buffer, wchans[i], &response_buffer);

    }

        // joining
    filethread.join();

    for(int i = 0; i<w; i++){
        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char *) &q, sizeof(q));
    }
    for(int i = 0; i < w; i++){
        workers[i].join();
    }*/

        /*for(int i = 0; i < h; i++){
            datamsg d(-1, 0, -1);
            response_buffer.push((char *)&d, sizeof(d));
        }*/

    else{
        //cout << "here at else" << endl;
        thread filethread(file_thread_function, fname, &request_buffer, chan, m);

        thread hists[h];
        for(int i = 0; i < h; i++){
            hists[i] = thread(histogram_thread_function, &response_buffer, &hc);

        }


        thread workers[w];
        for(int i = 0; i < w; i++){
            workers[i] = thread(worker_thread_function, m, &request_buffer, wchans[i], &response_buffer);

        }

        // joining
        filethread.join();

        for(int i = 0; i<w; i++){
            MESSAGE_TYPE q = QUIT_MSG;
            request_buffer.push((char *) &q, sizeof(q));
        }
        for(int i = 0; i < w; i++){
            workers[i].join();
        }


        for(int i = 0; i < h; i++){
            hists[i].join();
        }
        //hc.print ();
    }

    
    //cout << "about to join threads" << endl;
    // joining threads


    //cout << "patient threads finished" << endl;

    

    //cout << "worker threads finished" << endl;

    //Response r(-1, 0); // POTENTIAL ERROR: () was {}
    

	cout << "histogram threads finished. all client threads are now done" << endl;
    high_resolution_clock::time_point end_time = high_resolution_clock::now();
    double duration = duration_cast<microseconds>(end_time - start_time).count();
    cout << "time taken: " << duration/1000000 <<" seconds" << endl;
    // make worker channels
    /*FIFORequestChannel* wchans[w];
    for(int i = 0; i < w; i++){
        wchans[i] = create_new_channel(chan);
    }*/
	
    //struct timeval start, end;
    //gettimeofday (&start, 0);

    
	
    /*
    thread filethread (file_thread_function, fname, &request_buffer, chan, m);
    thread workers[w];
    for(int i = 0; i < w; i++){
        workers[i] = thread(worker_thread_function, wchans[i], &request_buffer, &hc, m);
    }*/

	/* Join all threads here */

    /*for(int i = 0; i < p; i++){
        patient[i].join();
    }*/

    //filethread.join();

    //cout << "file threads finished" << endl;

    /*for(int i = 0; i < w; i++){
        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char *) &q, sizeof(q));
        //workers[i].join();
    }
    for(int i = 0; i < w; i++){
        //MESSAGE_TYPE q = QUIT_MSG;
        //request_buffer.push((char *) &q, sizeof(q));
        workers[i].join();
    }*/

    //cout << "worker threads finished" << endl;

    //gettimeofday (&end, 0);
    // print the results
    //	hc.print ();

    /*
    for(int i = 0; i < p; i++){
        MESSAGE_TYPE q = QUIT_MSG;
        wchans[i]->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
        cout << "All Done!!!" << endl; 
        delete wchans[i];
    }*/

    /*int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;*/

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
    
}
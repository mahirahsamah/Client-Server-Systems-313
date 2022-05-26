
#ifndef _SHMreqchannel_H_
#define _SHMreqchannel_H_

#include "common.h"
#include "Reqchannel.h"
#include <semaphore.h>
#include <sys/mman.h>
#include <string>

using namespace std;

class SHMQ{
private:
	char* segment; // shared memory segment
	sem_t* sd; // sender done 
	sem_t* rd; // reciever done
	string name;
	int len;

public: 
	SHMQ (string _name, int _len): name (_name), len(_len){
		int fd = shm_open(name.c_str(), O_RDWR|O_CREAT, 0600);
		if( fd < 0){
			EXITONERROR ("Could not create/open shared memory segment");
		}
		ftruncate(fd,len);

		segment = (char*) mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED,fd,0);
		if(!segment){
			EXITONERROR("Could not map");
		}
		rd = sem_open((name + "_rd").c_str(), O_CREAT, 0600, 1); //initial value is 1
		sd = sem_open((name + "_sd").c_str(), O_CREAT, 0600,0); //initial value is 0
	}
	int my_shm_send(void* msg, int len){
		sem_wait (rd); 
		memcpy(segment, msg, len); 
		sem_post(sd); 
		return len;
	}
	int my_shm_recv (void* msg, int len){
		sem_wait (sd); 
		memcpy(msg, segment, len); 
		sem_post(rd); 
		return len;
	}

	//DESTRUCTOR
	~SHMQ (){

		sem_close(sd);
		sem_close(rd);
		sem_unlink((name + "_rd").c_str());
		sem_unlink((name + "_sd").c_str());

		//unmap shared memory segment
		munmap(segment, len);
		shm_unlink(name.c_str());


	}
};




class SHMRequestChannel: public RequestChannel{
private:
	SHMQ* shmq1;
	SHMQ* shmq2;
	int len;

public:
	SHMRequestChannel(const string _name, const Side _side, int _len);

	~SHMRequestChannel();

	int cread (void* msgbuf, int bufcapacity);
	
	int cwrite (void *msgbuf , int msglen);

};

#endif

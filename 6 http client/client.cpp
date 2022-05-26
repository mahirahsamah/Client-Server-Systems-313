#include <cassert>
#include <cstring>
#include <sstream>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <cstring>
#include <thread>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <math.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <unistd.h>
#include <netdb.h>
#include <fstream>
#include <sstream>
using namespace std;
#include <chrono>

string removepound(string url){
    //url.erase(std::remove(url.begin(), url.end(), '#'), url.end());
    //return ret;
    int startindex = url.find("#");
    string newurl = url.substr(0, startindex);
    return newurl;
}

// https://www.hello.com/path
string gethostname(string url) {
    url = removepound(url);

    int endHttp = url.find("//");

    int startIndex = url.find("//");
    string urlUpdated = url.substr(endHttp+2,url.length()-7);  //new url = www.hostname.com/path
    int endIndex = urlUpdated.find("/");

    string hostName = url.substr(startIndex+2, endIndex);
    return hostName;
}

string getpath(string url){
    //cout << "Getting path..." << endl;
    url = removepound(url);

    int endHttp = url.find("//");

    url = url.substr(endHttp+2,url.length()-7);  //new url = www.hostname.com/path
    int startIndex = url.find("/");
    int endIndex = url.length(); //until end of url

    string path;

    if(startIndex == string::npos){
        path = "/";
    }
    else {
        path = url.substr(startIndex, endIndex);
    }
    return path;
}

int hextodec(string hex){

    int result = 0;
    for (int i=0; i<hex.length(); i++) {
        if (hex[i]>=48 && hex[i]<=57)
        {
            result += (hex[i]-48)*pow(16,hex.length()-i-1);
        } else if (hex[i]>=65 && hex[i]<=70) {
            result += (hex[i]-55)*pow(16,hex.length( )-i-1);
        } else if (hex[i]>=97 && hex[i]<=102) {
            result += (hex[i]-87)*pow(16,hex.length()-i-1);
        }
    }
    return result;
}

string getfilename(string url){

    vector<int> indices;

    for(int i = 0; i < url.length(); i++){
        if(url.at(i) == '/'){
            indices.push_back(i);
        }
    }
    int sz = indices.size();
    int slashindex = indices[sz -1];

    string filename = url.substr(slashindex + 1);
    return filename;
}

void socket_function(string url, string fn){
    //string url = url;
    string filename = fn;

    string hostname = gethostname(url);
    string path  = getpath(url);
    string request = "GET " + path + " HTTP/1.1\r\nHost: " + hostname + "\r\nConnection: close\r\n\r\n";
    // tcp connection stuff

    struct addrinfo hints, *result;
    char s[INET6_ADDRSTRLEN];
    int status;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // socket
    int sockfd;

    string port = "80";

    if ((status = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result)) != 0) {
        cerr  << "getaddrinfo: " << gai_strerror(status) << endl;
        //return -1;
        cout << "get addr error" << endl;
        exit(-1); // could be return(1)
    }

    sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sockfd < 0) {
        perror("cannot create socket");
        exit(-1);
    }

    if (connect(sockfd, result->ai_addr, result->ai_addrlen) < 0) {
        perror("cannot connect");
        exit(-1);
    }
    if(send(sockfd, request.c_str(), request.size(), 0) == -1){ //send(sockfd, request.cstr(), request.size());
        perror("Client: Send failure");
        exit(-1);
    }

    // receive 
    char buf[2048];
    int bytes_read = 0;
    string outfilepath = "recv/" + filename;

    string header;
    std::stringstream ss;

    while((bytes_read = recv(sockfd, buf, 256, 0)) != 0){
        ss.write(buf, bytes_read);
    }
    std::string datastring = ss.str();
    int stringsize = datastring.size();
    

    // convert the header in ss_str into a vector of header lines
    vector<string> header_vec; 
    string token;

    string delim1 = "\r\n";
    string delim2 = "\r\n\r\n";
    int newline = 0;
    int doublenewline = datastring.find(delim2);
    int i =0;

    while( i < doublenewline){
        newline = datastring.find(delim1);
        string line = datastring.substr(0, newline);
        header_vec.push_back(line);
        i += newline + 2;
        datastring = datastring.substr(newline + 2);
    }
    string response_code = header_vec[0].substr(9, 3);

    bool tec_found = false;
    for(int i = 0; i < header_vec.size(); i++){
        //cout << header_vec[i] << endl;
        if(header_vec[i] == "transfer-encoding: chunked"){
            tec_found = true;
        }
    }


    close(sockfd);
    freeaddrinfo(result);

    if (response_code[0] == '2'){

        size_t pos = 0;
        size_t hexIndex = 0;
        string bodyLine;
        string delimiter = "\n";
        string delimiter2 = "\r\n";
        vector <string> hexNums;
        int startHexIndex = 0;
        int bytesPassed = 0;
        char buf[2048];
        int endIndex;
        int k = 0;
        int Index;
        
        //open file for writing
        ofstream outfile;
        outfile.open(filename, std::ios::binary); //opening file
        if(!outfile){
            cerr << "Error: file could not be opened." << endl;
            exit(1); 
        }
        
        if (tec_found){
            cout << "Chunk has been found" << endl;
            //concatenate chunks
            datastring = datastring.erase(0,6);//get rid of extra /n  and comments
            ///GET OF SIZE OF FIRST HEX
            string text = datastring.substr(2, 202);
            outfile << text;
            datastring = datastring.substr(204);
            vector<string> chunkVec;
            
            

            while((Index = datastring.find("\r\n"))!= string::npos){
                //while(hex value != 0){
                    k += 1;
                    if(k % 2 == 0){ //even-  looking at hex
                        cout <<"hexIndex: " << Index << endl;
                        datastring.erase(0, Index+2);  //3 = r\n
                    }
                    else { //k is odd - chunk
                
                        chunkVec.push_back(datastring.substr(0,Index));
                        datastring.erase(0,Index+3);
                    
                    }
                }
                stringstream stream;

                for(int i = 0 ; i< chunkVec.size(); i++){
                    //cout << chunkVec[i] << endl;
                    //cout << "---------------" << endl;
                    stream <<  chunkVec[i];

                        
                }
                
                string sstr = stream.str();
                cout << sstr << endl;
                outfile << sstr;
                outfile.close();
                
                



        }
        else{ //no chunks found
        
            datastring = datastring.substr(2);//get rid of extra /n
            while((pos = datastring.find("\n"))!= string::npos){
                bodyLine = datastring.substr(0,pos);
                outfile << bodyLine << endl;            
                datastring.erase(0, pos + delimiter.length());
            }    
            outfile.close(); 
            
        }
    }
    else if(response_code[0] == '3'){
        // get new hostname
        //cout << "here" << endl;
        for(int i = 0; i < header_vec.size(); i++){
            //cout << header_vec[i] << endl;
            if(header_vec[i].substr(0, 8) == "location"){
                
                url = header_vec[i].substr(10);

                socket_function(url, filename);
            }
        }
    }
    else if(response_code[0] == '4' || response_code[0] == '5') {
        cout << "Error: " << response_code << endl;
        exit(1);
    }
}

int main(int argc, char** argv) {

    // parsing command line input

    string url = argv[1];

    string filename = "";

    if(argc == 2){
        filename = getfilename(url);
    }
    else if(argc > 2){
        filename = argv[2];
    }

    socket_function(url, filename);    

}

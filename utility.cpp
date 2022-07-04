//
// Created by Cristiano on 2022/7/3.
//
#include "decoder.hpp"
using namespace std;
int last;
void read() {
    std::string str,strmem;
    std::cin>>str;
    int cnt=0,k=0;
    while(str=="end"||!std::cin.eof()){
        if(str[0]=='@'){
            str=str.substr(1);
        }
        else{
            cnt++;
        }
        if(!(std::cin>>str)){
            last=cnt;break;
        }
        if(str=="end"){
            last=cnt;break;
        }
    }
}
int main(){
    read();
    switch (last) {
        
    }


}

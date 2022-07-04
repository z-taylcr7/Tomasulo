//
// Created by Cristiano on 2022/6/21.
//
#include <iostream>
#ifndef RISC_V_SIMULATOR_UTILITY_HPP
#define RISC_V_SIMULATOR_UTILITY_HPP
namespace Cristiano{
    std::string reverse(const std::string &str){
        int l=str.length();
        char ans[l+1];
        for(int i=l-1;i>=0;i--)ans[l-1-i]=str[i];
        ans[l]='\0';
        return( std::string )(ans);
    }
    int btoi(const std::string str){
        int l=str.length();int ans=0;
        for(int i=0;i<l;i++){
            ans=ans*2+str[i]-'0';
        }
        return ans;
    }
    int xtoi(const std::string str){
        int l=str.length();int ans=0;
        for(int i=0;i<l;i++){
            char i1=str[i];int in1=0;
            if (i1 >= 'A' && i1 <= 'F')in1 = i1 - 'A' + 10;
            else in1 = i1 - '0';
            ans=ans*16+in1;
        }
        return ans;
    }

    inline unsigned int sext(const unsigned int &x, const int &r){
        bool c=((unsigned)1<<r)&x;
        if(!c)return x;
        return x|((-1)<<(r+1));
    }
    int fix(int x,int h){
        if ((x >> h) & 1) {
            for (int j = h + 1; j < 32; j++) {
                x += 1 << j;
            }
        }
        return x;
    }
    std::string decToBin(unsigned int u){
        std::string str;
        while(u>0){
            int a=u&1;u=u>>1;char ch='0';
            if(a)ch='0'+a;
            str=ch+str;
        }
        int l=str.length();
        for(int i=0;i<32-l;i++)str='0'+str;
        return str;
    }
    std::string decToHex(int u){
        std::string str;
        while(u>0){
            int a=u%16;u=u/16;char ch;
            if (a >= 10 && a <= 15)ch = a - 10  +'A';
            else ch ='0'+a;
            str=ch+str;
        }
        return str;
    }
    inline unsigned int ask(const unsigned int &x,const int &l,const int &r){
        return (x&( ((-1)>>(31-r+l))<<l ))>>l;
    }
     int hexStringToDec(std::string str){
        int ret = 0;
        for (int i = 0; i < str.length(); ++i) ret = (ret << 4) + ((str[i]>='A'&&str[i]<='F') ? (str[i] - 'A' + 10) : (str[i] - '0'));
        return ret;
    }




    template<class T,int len=32>
    class Que{
    private:

        //1 for full,-1 for isEmpty, others 0
        char status;
        T que[len];
    public:
        int size;int head, tail;

        int reserve() {
            ++size;
            int preTail = tail;
            tail++;
            if (tail == len) tail = 0;
            if (status == -1) status = 0;
            if (tail == head) status = 1;
            return preTail + 1;
        }

        int getTail() { return tail + 1; }

        Que() : head(0), tail(0), status(-1), size(0) {
            memset(que, 0, sizeof(que));
        }

        char getStatus() { return status; }

        int push(const T &t) {
            ++size;
            int prePail = tail;
            que[tail++] = t;
            if (tail == len) tail = 0;
            if (status == -1) status = 0;
            if (tail == head) status = 1;
            return prePail + 1;
        }

        void pop() {
            --size;
            if (++head == len) head = 0;
            if (status == 1) status = 0;
            if (head == tail) status = -1;
        }

        T &top() { return que[head]; }

        T &operator[](int pos) { return que[pos - 1]; }

        void clear() {
            memset(que, 0, sizeof(que));
            head = tail = 0;
            size = 0;
            status = -1;
        }
    };


}
#endif //RISC_V_SIMULATOR_UTILITY_HPP

#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <iostream>
#include <cstring>
#include <vector>
#include "utility.hpp"
#include "decoder.hpp"
namespace Cristiano {


    int rob_entry_tag;
    int rs_entry_tag;
    int lb_entry_tag;
    const int QUEUE_SIZE = 32;
    const int mem_size = 4194304;

    unsigned int mem[mem_size];


    struct issueReturn{
        bool available;
        bool gotoRS,gotoSLB;
        instruction ins;
        RS_Node node;
        issueReturn():available(0),gotoRS(0),gotoSLB(0){}
    }isReturn;
    struct executeReturn{
        bool available;
        uint value,pc_cur;
        int rob_tag;
        executeReturn():available(0),value(0),pc_cur(0),rob_tag(0){}
    }exReturn;
    struct radiator{
        int rd_issue,pos,rd_commit;
        int index;
        uint val;
        radiator():rd_issue(-1),pos(-1),rd_commit(-1),val(0){}
        void reset(){
            rd_issue=rd_commit=pos=-1;
            val=0;
        }
    }channel;


    class simulator {
    private:
        char predict[4096],history[4096];
        char predictTable[4096][4];
        //在这里保留了助教实现时使用的变量，仅供参考，可以随便修改，推荐全部删除。

        //inst_fetch_and_queue
        class instFetchQueue {
            struct node{
                instruction ins;
                int pc_n;
            }queue[32];
            int head,tail;

        public:
            int size=0;
            instFetchQueue(){
                head=1;tail=1;size=0;
            }
            ~instFetchQueue()=default;

            bool enQueue(const instruction& i,int Pc){
                if(size==32)return false;
                size++;
                queue[head++]={i,Pc};
                if(head==QUEUE_SIZE)head=0;
                if(head==tail)std::cout<<"inst queue full!!"<<std::endl;
                return true;
            }
            node& top(){
                int pos=head==0?31:(head-1);
                return queue[pos];
            }

            void deQueue(){
                size--;
                if(tail==head)return ;
                tail++;if(tail==QUEUE_SIZE)tail=0;
            }
            void flush(){
                head=tail=0;
                memset(queue,0,QUEUE_SIZE);
            }
        }IFQueuePrev,IFQueueNext;
        //issue
        //reservation
        class Reservation{
        public:
            struct ResultBuffer {
                int head;
                RS_Node rsQue[QUEUE_SIZE];
                ResultBuffer() : head(0) {
                    for (int i = 0; i < QUEUE_SIZE; ++i) rsQue[i].next = i + 1;
                }

                void clear() {
                    head = 0;
                    for (int i = 0; i < QUEUE_SIZE; ++i) {
                        rsQue[i].clear();
                        rsQue[i].next = i + 1;
                    }
                }
            } preBuffer, nextBuffer;

            RS_Node exNode;
            bool exFlag;

            //scan the whole station and return the command which is ready for execution
            bool scan() {
                for (int j = 0; j < QUEUE_SIZE; ++j)
                    if (preBuffer.rsQue[j].busy && preBuffer.rsQue[j].executable()) {
                        exNode = preBuffer.rsQue[j];
                        remove(j);
                        return true;
                    }
                return false;
            }

            void update() { preBuffer = nextBuffer; }

            void insert(const RS_Node &rsNode) {
                int next = nextBuffer.rsQue[nextBuffer.head].next;
                nextBuffer.rsQue[nextBuffer.head] = rsNode;
                nextBuffer.head = next;
            }

            void remove(int pos) {
                nextBuffer.rsQue[pos].clear();
                nextBuffer.rsQue[pos].next = nextBuffer.head;
                nextBuffer.head = pos;
            }

            //get the value in pre
            RS_Node &operator[](int pos) { return preBuffer.rsQue[pos]; }

            //get the value in next
            RS_Node &operator()(int pos) { return nextBuffer.rsQue[pos]; }

            bool isFull() { return nextBuffer.head == QUEUE_SIZE; }

            void flush() {
                exFlag = false;
                preBuffer.clear();
                nextBuffer.clear();
            }


            Reservation() : exFlag(0) {}
        }rs;
        //slbuffer
        struct SLBuffer {
            bool hasResult, isStore, newHasResult, newIsStore;
            unsigned int value, newValue;
            int posROB, newPosROB;

            SLBuffer() : hasResult(false), isStore(false), value(0), posROB(0),
                         newHasResult(false), newIsStore(false), newValue(0), newPosROB(0) {}

            Que<SLB_Node> preQue, nextQue;

            void update() {
                preQue = nextQue;
                value = newValue;
                hasResult = newHasResult;
                isStore = newIsStore;
                posROB = newPosROB;
            }

            bool isFull() { return nextQue.getStatus() == 1; }

            bool isEmpty() { return preQue.getStatus() == -1; }

            void push(const SLB_Node &node) { nextQue.push(node); }

            void pop() { nextQue.pop(); }

            SLB_Node &operator[](int pos) { return preQue[pos]; }

            SLB_Node &operator()(int pos) { return nextQue[pos]; }

            void traverse_commit(int posRob) {
                int i = nextQue.head;
                bool flag = true;
                while (i != nextQue.tail || flag && nextQue.getStatus() == 1) {
                    flag = false;
                    if (nextQue[i + 1].node.id == posRob) {
                        nextQue[i + 1].has_commit = true;
                    }
                    ++i;
                    if (i == QUEUE_SIZE) i = 0;
                }
            }

            void traverse(int posRob, unsigned int val) {
                int i = nextQue.head;
                bool flag = true;
                while (i != nextQue.tail || flag && nextQue.getStatus() == 1) {
                    flag = false;
                    if (nextQue[i + 1].type == S) {
                        if (nextQue[i + 1].node.Q1 == posRob) {
                            nextQue[i + 1].node.Q1 = 0;
                            nextQue[i + 1].node.V1 = val;
                        }
                        if (nextQue[i + 1].node.Q2 == posRob) {
                            nextQue[i + 1].node.Q2 = 0;
                            nextQue[i + 1].node.V2 = val;
                        }
                    } else {
                        if (nextQue[i + 1].node.Q1 == posRob) {
                            nextQue[i + 1].node.Q1 = 0;
                            nextQue[i + 1].node.V1 = val;
                        }
                    }
                    ++i;
                    if (i == QUEUE_SIZE) i = 0;
                }
            }

            void clear() {
                newHasResult = false;
                std::vector<SLB_Node> vec_slbuffer;
                while (nextQue.getStatus() != -1 && nextQue.top().type == S &&
                       nextQue.top().ready()) {
                    vec_slbuffer.push_back(nextQue.top());
                    nextQue.pop();
                }
                nextQue.clear();
                for (int i = 0; i < vec_slbuffer.size(); ++i) nextQue.push(vec_slbuffer[i]);
                preQue = nextQue;
            }
        }slb;
        //rob
        class ROB{
        private:
            Que<ROB_Node,QUEUE_SIZE>Que_prev,Que_next;
        public:
            bool isFull() { return Que_next.getStatus() == 1; }

            void update() { Que_prev = Que_next; }

            int getTail() { return Que_prev.getTail(); }

            int reserve() {
                int pos = Que_next.reserve();
                Que_next[pos].index = pos;
                return pos;
            }

            ROB_Node &top() { return Que_prev.top(); }

            ROB_Node &operator[](int pos) { return Que_prev[pos]; }

            ROB_Node &operator()(int pos) { return Que_next[pos]; }

            void pop() { Que_next.pop(); }

            void flush() {
                Que_prev.clear();
                Que_next.clear();
            }

            bool isEmpty() {
                return Que_prev.getStatus() == -1;
            }

            int getSize() {
                return Que_prev.size;
            }
            bool checkCacheRAW(int offset){
                int j=Que_next.head;
                while(j!=Que_next.tail){
                    if(Que_next[j].ins.type==S&&Que_next[j].ins.rd==offset)return false;
                    j++;
                    if(j==QUEUE_SIZE)j=1;
                }
                return true;
            }
        }rob;
        //commit

    public:
        uint pc,pc_next,rs_pc_prev,rs_pc_predict,predictSuccess,predictFail;
        instruction rob_to_commit,rs_ins;
        int link_to_reg,commit_slb_index;
        bool rs_stall,rob_stall,slb_stall,commit_to_slb;
        bool tmsl_reserve,tmsl_fetch,tmsl_commit,is_to_ex,new_is_to_ex,finish=false;
        RS_Node is_to_ex_node,new_is_to_ex_node;
        codeType rs_type;

        simulator() {
            //memory
            memset(mem,0,sizeof(mem));

            //pc
            pc_next= 0;pc=0;

            //regfile
            rs_stall=false,rob_stall=false,slb_stall=false;

            //inst_fetch_and_queue
            new_is_to_ex = false;
            is_to_ex = false;
            tmsl_reserve = false;
            tmsl_fetch = false;
            tmsl_commit=false;
            rs.exFlag = false;
            slb.hasResult = false;
            rob_stall = rs_stall = slb_stall = false;

            //issue

            //reservation

            //ex

            //slbuffer

            //rob

            //commit
            for (int i = 0; i < 4096; ++i) predict[i] = 1;
            for (int i = 0; i < 4096; ++i)
                for (int j = 0; j < 3; ++j) predictTable[i][j] = 2;
            memset(history, 0, sizeof(history));
        }

        void scan() {
            std::string str,strmem;
            std::cin>>str;
            int cnt=0;
            while(str=="end"||!std::cin.eof()){
                if(str[0]=='@'){
                    str=str.substr(1);cnt=hexStringToDec(str);
                }
                else{
                    int num=(hexStringToDec(str));
                    mem[cnt]|=num;cnt++;
                }
                if(!(std::cin>>str)){
                    last=cnt;break;
                }
                if(str=="end"){
                    last=cnt;break;
                }
            }
        }
        unsigned int cycle = 0;

        void run() {
            while (true) {
                /*在这里使用了两阶段的循环部分：
                  1. 实现时序电路部分，即在每个周期初同步更新的信息。
                  2. 实现逻辑电路部分，即在每个周期中如ex、issue的部分
                  已在下面给出代码
                */
                cycle++;

                run_rob();
                if(rob_to_commit.PC==8||
                   rob_to_commit.op==lui&&rob_to_commit.immediate.value==255&&rob_to_commit.rd==0){
                    std::cout << std::dec << ((unsigned int)reg_prev[10] & 255u);
                    break;
                }
                run_slbuffer();
                run_reservation();
                run_regfile();
                run_inst_fetch_queue();
                update();
                run_issue();
                run_ex();
                run_commit();
                showReg();
                //if(cycle%100000==0)std::cout<<cycle<<std::endl;
            }
        }
        void showReg(){
            //std::cout<<orders[ins[0].command]<<" has just completed "<<std::endl;
            std::cout<<decToHex(pc)<<std::endl;
            for(int i=0;i<32;i++){std::cout<<"reg["<<i<<']'<<" = "<<reg_prev[i]<<"   ";if(i%8==7)std::cout<<std::endl;}
        }
        void run_inst_fetch_queue() {
            /*
            在这一部分你需要完成的工作：
            1. 实现一个先进先出的指令队列
            2. 读取指令并存放到指令队列中
            3. 准备好下一条issue的指令
            tips: 考虑边界问题（满/空...）
            */
            //if(finish)return;
            pc=pc_next;
            if(tmsl_fetch)IFQueueNext.deQueue();
            if(IFQueueNext.size==32)return;
            std::string line;
            if(fetchCode(pc,line)==-1)finish=true;
            instruction i= decode(line);

            if(i.op==jal){
                pc_next=pc+sext(i.immediate.value,i.immediate.height);
            }else if (i.type == B) {
                pc_next= (predictTable[pc][history[pc]] >= 2) ? pc + i.immediate.value : pc + 4;
            } else pc_next = pc + 4;
            i.PC=pc;
            IFQueueNext.enQueue(i,pc_next);
        }

        void run_issue() {
            tmsl_fetch= false;
            tmsl_reserve=false;
            isReturn.available=false;
            if(IFQueuePrev.size>0&&!rob_stall){
                instruction p=IFQueuePrev.top().ins;
                tmsl_fetch=true;
                isReturn.gotoSLB=isReturn.gotoRS=false;
                bool illegal=(p.type==E||p.op==end);
                if(illegal)return;
                isReturn.available= true;
                int posrob=rob.getTail();
                if(p.rd!=-1&&p.rd!=0){
                    channel.rd_issue=p.rd;
                    channel.pos=posrob;
                }else{
                    channel.rd_issue=-1;
                }
                int qj=0,qk=0,vj=0,vk=0;
                if(p.rs1!=-1){
                    if((qj=reg_prev(p.rs1))==0)vj=reg_prev[p.rs1];
                    else if(rob[qj].has_value){
                        vj=rob[qj].value;qj=0;
                    }
                }else qj=0;
                if(p.rs2!=-1){
                    if((qk=reg_prev(p.rs2))==0)vk=reg_prev[p.rs2];
                    else if(rob[qk].has_value){
                        vk=rob[qk].value;qk=0;
                    }
                }else qk=0;
                isReturn.node.setValue(p,true,posrob,qj,qk,vj,vk);
                isReturn.gotoSLB=isSL(p.op);
                isReturn.gotoRS=!isReturn.gotoSLB;
                tmsl_reserve=true;
                rs_pc_prev=IFQueuePrev.top().ins.PC;
                rs_ins=p;
                rs_type=isReturn.node.ins.type;
                link_to_reg=(p.rd==0)?-1:p.rd;//reg[0]=0, never ever update!
                rs_pc_predict=IFQueuePrev.top().pc_n;
                new_is_to_ex=false;
                isReturn.node.ins=rs_ins;
                if(isReturn.gotoRS&&isReturn.node.executable()){
                    isReturn.available=false;
                    new_is_to_ex=true;
                    new_is_to_ex_node=isReturn.node;
                }
                if (isReturn.gotoRS && rs_stall || isReturn.gotoSLB && slb_stall) {
                    isReturn.available = false;
                    tmsl_reserve = false;
                    tmsl_fetch = false;
                }
            }
            /*
            在这一部分你需要完成的工作：
            1. 从run_inst_fetch_queue()中得到issue的指令
            2. 对于issue的所有类型的指令向ROB申请一个位置（或者也可以通过ROB预留位置），并修改regfile中相应的值
            2. 对于 非 Load/Store的指令，将指令进行分解后发到Reservation Station
              tip: 1. 这里需要考虑怎么得到rs1、rs2的值，并考虑如当前rs1、rs2未被计算出的情况，参考书上内容进行处理
                   2. 在本次作业中，我们认为相应寄存器的值已在ROB中存储但尚未commit的情况是可以直接获得的，即你需要实现这个功能
                      而对于rs1、rs2不ready的情况，只需要stall即可，有兴趣的同学可以考虑下怎么样直接从EX完的结果更快的得到计算结果
            3. 对于 Load/Store指令，将指令分解后发到SLBuffer(需注意SLBUFFER也该是个先进先出的队列实现)
            tips: 考虑边界问题（是否还有足够的空间存放下一条指令）
            */
            /*if(rob.getSize()>QUEUE_SIZE) {
                if(cur.rd!=-1&&reg(cur.rd)>0)issue=false;pc_next=pc;return;
            }
            rob_entry_tag=rob.push_back(cur);
            if(cur.op==lb||cur.op==lbu||cur.op==lh||cur.op==lhu||cur.op==lw){
                if(LoadBuffer.size>=QUEUE_SIZE){issue=false;return;}
                LoadBuffer.push_back(cur);
                if(reg(cur.rs1)>0){
                    h=reg(cur.rs1);
                    if(rob[h].ready){
                        LoadBuffer.que[lb_entry_tag].vj=rob[h].value;
                        LoadBuffer.que[lb_entry_tag].qj=0;
                    }
                    else LoadBuffer.que[lb_entry_tag].qj=h;
                }else{
                    LoadBuffer.que[lb_entry_tag].vj=reg[cur.rs1];
                    LoadBuffer.que[lb_entry_tag].qj=0;
                }

                /*
                if(reg[cur.rs2)>0){
                    h=reg[cur.rs2);
                    if(rob[h].ready){
                        LoadBuffer.que[lb_entry_tag].vk=rob[h].value;
                        LoadBuffer.que[lb_entry_tag].qk=0;
                    }
                    else LoadBuffer.que[lb_entry_tag].qk=h;
                }else{
                    LoadBuffer.que[lb_entry_tag].vk=reg[cur.rs2];
                    LoadBuffer.que[lb_entry_tag].qk=0;
                }


            }else {
                rs.addIns(cur);
                cur.PC=pc;
                rs.st[rs_entry_tag].A=cur.immediate;

                if(reg(cur.rs1)>0){
                    h=reg(cur.rs1);
                    if(rob[h].ready)rs.updateRS(rs_entry_tag,rob[h].value);
                    else rs.st[rs_entry_tag].qj=h;
                }else{
                    rs.st[rs_entry_tag].vj=reg[cur.rs1];
                    rs.st[rs_entry_tag].qj=0;
                }
                if(!(cur.type==S||cur.op==lb||cur.op==lbu||cur.op==lh||cur.op==lhu||cur.op==lw)) {
                    if(reg(cur.rs2)>0){
                        int h=reg[cur.rs2];
                        if(rob[h].ready)rs.updateRS(rs_entry_tag,rob[h].value);
                        else rs.st[rs_entry_tag].qk=h;
                    }else{
                        rs.st[rs_entry_tag].vk=reg[cur.rs2];
                        rs.st[rs_entry_tag].qk=0;
                    }
                }
            }
            if(cur.rd!=-1){
                reg(cur.rd)=rob_entry_tag;
            }
        */
        }
        void run_reservation() {
            /*
            在这一部分你需要完成的工作：
            1. 设计一个Reservation Station，其中要存储的东西可以参考CAAQA或其余资料，至少需要有用到的寄存器信息等
            2. 如存在，从issue阶段收到要存储的信息，存进Reservation Station（如可以计算也可以直接进入计算）
            3. 从Reservation Station或者issue进来的指令中选择一条可计算的发送给EX进行计算
            4. 根据上个周期EX阶段或者SLBUFFER的计算得到的结果遍历Reservation Station，更新相应的值
            */
            if(isReturn.available&&isReturn.gotoRS){
                rs.insert(isReturn.node);
            }
            rs.exFlag=rs.scan();
            if(rs.exFlag&&new_is_to_ex){
                rs.insert(new_is_to_ex_node);
                new_is_to_ex=false;
            }
            //exReturn from last time
            if(exReturn.available){
                for(int i=0;i<QUEUE_SIZE;++i){
                    if(rs(i).busy&&rs(i).match(exReturn.rob_tag))
                        rs(i).setValue(exReturn.rob_tag,exReturn.value);
                }
            }
            if(slb.hasResult){
                for(int i=0;i<QUEUE_SIZE;++i){
                    if(rs(i).busy&&rs(i).match(slb.posROB))
                        rs(i).setValue(slb.posROB,slb.value);
                }
            }
            rs_stall=rs.isFull();
        }

        void run_ex() {
            /*
            在这一部分你需要完成的工作：
            根据Reservation Station发出的信息进行相应的计算
            tips: 考虑如何处理跳转指令并存储相关信息
                  Store/Load的指令并不在这一部分进行处理
            */
            exReturn.available=true;
            if(is_to_ex){
                RS_Node& tmp=is_to_ex_node;
                exReturn.rob_tag=tmp.id;
                ALU alu(tmp.ins);
                alu.execute(tmp.V1,tmp.V2);
                exReturn.value=alu.data;
                exReturn.pc_cur=Cristiano::pc;
                is_to_ex=false;
            }else if(rs.exFlag){
                RS_Node &tmp=rs.exNode;
                exReturn.rob_tag=tmp.id;
                ALU alu(tmp.ins);
                alu.execute(tmp.V1,tmp.V2);
                exReturn.value=alu.data;
                exReturn.pc_cur=Cristiano::pc;
                rs.exFlag=false;
            }else{
                exReturn.available=false;
            }
        }

        void run_slbuffer() {
            /*
            在这一部分中，由于SLBUFFER的设计较为多样，在这里给出两种助教的设计方案：
            1. 1）通过循环队列，设计一个先进先出的SLBUFFER，同时存储 head1、head2、tail三个变量。
               其中，head1是真正的队首，记录第一条未执行的内存操作的指令；tail是队尾，记录当前最后一条未执行的内存操作的指令。
               而head2负责确定处在head1位置的指令是否可以进行内存操作，其具体实现为在ROB中增加一个head_ensure的变量，每个周期head_ensure做取模意义下的加法，直到等于tail或遇到第一条跳转指令，
               这个时候我们就可以保证位于head_ensure及之前位置的指令，因中间没有跳转指令，一定会执行。因而，只要当head_ensure当前位置的指令是Store、Load指令，我们就可以向slbuffer发信息，增加head2。
               简单概括即对head2之前的Store/Load指令，我们根据判断出ROB中该条指令之前没有jump指令尚未执行，从而确定该条指令会被执行。

               2）同时SLBUFFER还需根据上个周期EX和SLBUFFER的计算结果遍历SLBUFFER进行数据的更新。

               3）此外，在我们的设计中，将SLBUFFER进行内存访问时计算需要访问的地址和对应的读取/存储内存的操作在SLBUFFER中一并实现，
               也可以考虑分成两个模块，该部分的实现只需判断队首的指令是否能执行并根据指令相应执行即可。

            2. 1）SLB每个周期会查看队头，若队头指令还未ready，则阻塞。

               2）当队头ready且是load指令时，SLB会直接执行load指令，包括计算地址和读内存，
               然后把结果通知ROB，同时将队头弹出。ROB commit到这条指令时通知Regfile写寄存器。

               3）当队头ready且是store指令时，SLB会等待ROB的commit，commit之后会SLB执行这
               条store指令，包括计算地址和写内存，写完后将队头弹出。
            */
            /*if(LoadBuffer.size==0)return false;

            LoadBuffer.pop();
            instruction i=LoadBuffer.que[lb_entry_tag].ins;
            rob.push_back(i);
                if(reg(i.rs1)==0){
                   // i.immediate.value+=reg[i.rs1].value;LoadBuffer.que[lb_entry_tag].stepped= true;
                    if(rob.checkCacheRAW(rs[rs_entry_tag].ins.rd)){
                        ALU aluload(i);
                        aluload.execute();
                        LoadBuffer.que[lb_entry_tag].ready=true;
                        rob[reg(i.rd)].value=aluload.data;
                        rob[reg(i.rd)].ready= true;
                        //LoadBuffer.que[lb_entry_tag].stepped= false;
                }
            }
            LoadBuffer.commit();
             */
            //push back issueReturn
            slb.newHasResult= false;
            if(isReturn.available&&isReturn.gotoSLB){
                SLB_Node t;
                t.node=isReturn.node;
                if(slb.isEmpty()){
                    if(t.ready())t.count++;
                }
                t.type=isReturn.node.ins.type;
                slb.push(t);
            }
            //pop out one and execute it
            if(!slb.isEmpty()){
                SLB_Node &tmp=slb.preQue.top();
                if(tmp.ready()){
                    tmp.count++;
                    if(tmp.count==3){
                        ALU alu(tmp.node.ins);
                        alu.execute(tmp.node.V1,tmp.node.V2);
                        if(tmp.type==S){
                            alu.writeMemory();
                            slb.newIsStore=true;
                            slb.newHasResult=false;
                        }else{
                            tmp.node.V2=alu.data;
                            slb.newValue=tmp.node.V2;
                            slb.newIsStore=false;
                            slb.newHasResult=true;
                        }
                        slb.newPosROB=tmp.node.id;
                        slb.pop();
                    }else{
                        slb.nextQue.top()=tmp;
                    }
                }
            }
            if(commit_to_slb)slb.traverse_commit(commit_slb_index);
            if(exReturn.available)slb.traverse(exReturn.rob_tag,exReturn.value);
            if(slb.hasResult&&!slb.isStore)slb.traverse(slb.posROB,slb.value);
            slb_stall=slb.isFull();
        }


        void run_rob() {
            /*
            在这一部分你需要完成的工作：
            1. 实现一个先进先出的ROB，存储所有指令
            1. 根据issue阶段发射的指令信息分配空间进行存储。
            2. 根据EX阶段和SLBUFFER的计算得到的结果，遍历ROB，更新ROB中的值
            3. 对于队首的指令，如果已经完成计算及更新，进行commit
            */

            //top commit
            if(tmsl_reserve){
                int index=rob.reserve();
                rob(index).ins=rs_ins;
                if(rs_ins.type==S)rob(index).has_value=true;
                rob(index).pc_prev=rs_pc_prev;
                rob(index).dest=link_to_reg;
                rob(index).pc_predict=rs_pc_predict;
                tmsl_reserve=false;
            }
            if(tmsl_commit){
                rob.pop();
                tmsl_commit=false;
            }
            if(exReturn.available){
                ROB_Node &node =rob(exReturn.rob_tag);
                node.has_value=true;
                node.value=exReturn.value;
                node.pc_now=exReturn.pc_cur;
            }
            if(slb.hasResult&&!slb.isStore){
                ROB_Node &tmp = rob(slb.posROB);
                tmp.has_value = true;
                tmp.value = slb.value;
            }
            rob_stall=rob.isFull();
        }
        void run_regfile(){
            //update registers according to data from channel
            if (channel.rd_commit != -1) {
                reg_next[channel.rd_commit] = channel.val;
                if (reg_next(channel.rd_commit) == channel.index)reg_next(channel.rd_commit) = 0;
            }
            if (channel.rd_issue != -1) {
                reg_next(channel.rd_issue) = channel.pos;
            }
            channel.reset();
        }

        void run_commit() {
            /*
            在这一部分你需要完成的工作：
            1. 根据ROB发出的信息更新寄存器的值，包括对应的ROB和是否被占用状态（注意考虑issue和commit同一个寄存器的情况）
            2. 遇到跳转指令更新pc值，并发出信号清空所有部分的信息存储（这条对于很多部分都有影响，需要慎重考虑）
            */
            commit_to_slb = false;
            tmsl_commit = false;



            //JUMP!
            if (rob.getSize() == 0)return;
            ROB_Node it = rob.top();
            if (it.has_value) {
                commit_to_slb = it.ins.type == S;
                commit_slb_index = it.index;
                channel.rd_commit = it.dest;
                channel.val = it.value;
                channel.index = it.index;
                tmsl_commit = true;
                rob_to_commit = it.ins;
                if (it.ins.type == B) {
                    if (it.pc_now != it.pc_prev + 4) {
                        if (predictTable[it.pc_prev][history[it.pc_prev]] < 3)
                            ++predictTable[it.pc_prev][history[it.pc_prev]];
                    } else if (predictTable[it.pc_prev][history[it.pc_prev]])
                        --predictTable[it.pc_prev][history[it.pc_prev]];
                    history[it.pc_prev] = (history[it.pc_prev] << 1) | (it.pc_now != it.pc_prev + 4);
                    history[it.pc_prev] &= 0b11;
                    if (it.pc_predict == it.pc_now) ++predictSuccess;
                    else ++predictFail;
                }
                if (( it.ins.op == jalr||it.ins.type==B) && it.pc_predict != it.pc_now) {
                    isReturn.available = false;
                    new_is_to_ex = false;
                    is_to_ex = false;
                    tmsl_reserve = false;
                    tmsl_fetch = false;
                    exReturn.available = false;
                    rs.exFlag = false;
                    slb.hasResult = false;
                    rob_stall = rs_stall = slb_stall = false;
                    rob.flush();
                    rs.flush();
                    slb.clear();
                    IFQueuePrev.flush();
                    IFQueueNext.flush();
                    reg_next.flushOrder();
                    reg_prev.flushOrder();
                    channel.reset();
                    pc_next = it.pc_now;
                    tmsl_commit = false;
                }
            }
        }

        void update() {
            /*
            在这一部分你需要完成的工作：
            对于模拟中未完成同步的变量（即同时需记下两个周期的新/旧信息的变量）,进行数据更新。
            */
            reg_prev=reg_next;
            IFQueuePrev=IFQueueNext;
            is_to_ex=new_is_to_ex;
            is_to_ex_node=new_is_to_ex_node;
            rs.update();
            slb.update();
            rob.update();
        }



        ~simulator() {  }
    };
}

#endif
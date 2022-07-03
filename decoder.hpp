//
// Created by Cristiano on 2022/6/21.
//
#include "utility.hpp"
#ifndef RISC_V_SIMULATOR_DECODER_HPP
#define RISC_V_SIMULATOR_DECODER_HPP

namespace Cristiano {
    struct im{
        int value;
        int height=31;
    };
    uint pc=0;
    uint last=0;
    enum codeType {
        U, J, I, B, S, R
    };
    enum opcode {
        add, sub, sll, slt, sltu, Xor, srl, sra, Or, And,//R
        lui, auipc,//U
        jal, beq, bne, blt, bge, bltu, bgeu,//B
        lb, lh, lw, lbu, lhu, addi, slti, sltiu, xori, ori, andi, slli, srli, srai,//I
        jalr,//J
        sb, sh, sw,//S
        end
    };
    struct instruction{
        int rs1=-1;int rs2=-1;int rd=-1;
        im immediate={-1,0};opcode op=end;int shamt=-1;codeType type;
        int PC=-1;
        instruction(){
            rs1=rs2=rd=0;immediate={0,0};shamt=0;
            op=add;
        }
        instruction(const int& r1,const int& r2,int d,im imm,opcode oc):rs1(r1),rs2(r2),rd(d),immediate(imm),op(oc){};

        bool operator==(const instruction &rhs) const {
            return rs1 == rhs.rs1 &&
                   rs2 == rhs.rs2 &&
                   rd == rhs.rd &&
                   immediate.value == rhs.immediate.value &&
                   op == rhs.op &&
                   shamt == rhs.shamt;
        }

        bool operator!=(const instruction &rhs) const {
            return !(rhs == *this);
        }
    };
    struct regFile {
        uint value[32] = {0};
        int reorder[32] = {0};

        regFile() {
            memset(reorder, 0, sizeof(reorder));
            memset(value, 0, sizeof(value));
        }

        uint &operator[](int pos) {
            return value[pos];
        }

        int &operator()(int pos) {
            return reorder[pos];
        }

        void flushOrder() {
            memset(reorder, 0, sizeof(reorder));
        }
    }reg_prev,reg_next;
    const int COUNTER=111;
    struct Counter {
        int b;
        int his;
        int counter[4];
        Counter()=default;
    }counter[COUNTER];

    struct ROB_Node{
        bool has_value;
        uint value;
        int pc_predict,pc_now,pc_prev;
        instruction ins;
        int dest,index;
        bool ready;
        int counter;

    };
    struct RS_Node{
        bool busy;instruction ins;
        int id, Q1, Q2, next;
        unsigned int V1, V2;


        RS_Node() : busy(0) {}

        void setValue(const instruction &i,bool Busy, int Id, int QI, int QII, unsigned int VI, unsigned int VII) {
            ins=i;
            busy = Busy, id = Id, Q1 = QI, Q2 = QII, V1 = VI, V2 = VII;
        }

        bool executable() {
            return (Q1 == 0 && Q2 == 0);
        }

        bool match(int pos) {
            return (Q1 == pos || Q2 == pos);
        }

        void setValue(int pos, unsigned int v) {
            if (Q1 == pos) {
                V1 = v;
                Q1 = 0;
            }
            if (Q2 == pos) {
                V2 = v;
                Q2 = 0;
            }
        }

        void clear() {
            busy = 0;
            Q1 = Q2 = V1 = V2 = next = 0;
        }
    };
    struct SLB_Node{
        int count;
        codeType type;
        RS_Node node;
        bool has_commit;
        SLB_Node():count(0),has_commit(false){}
        bool ready(){
            if(type==S){
                return(node.Q1==0&&node.Q2==0&&has_commit);
            }else{
                return node.Q1==0;
            }
        }
    };
    extern uint mem[4194304];

    uint readMemory(int pc);
    codeType judgeType(const std::string &o) {
        if (o == "0110111" || o == "0010111")return U;
        if (o == "1101111")return J;
        if (o == "1100111" || o == "0010011")return I;
        if (o == "0000011")return I;
        if (o == "1100011")return B;
        if (o == "0100011")return S;
        if (o == "0110011")return R;
        return R;
    };
    instruction Rdecode(const std::string &order) {
        int rd = btoi(order.substr(20, 5));
        int rs1 = btoi(order.substr(12, 5));
        int rs2 = btoi(order.substr(7, 5));
        opcode command;
        std::string fun(order.substr(17, 3));
        int x = order[1] - '0';
        if (fun == "000") {
            command = x ? sub : add;
        }
        if (fun == "001")command = sll;
        if (fun == "010")command = slt;
        if (fun == "011")command = sltu;
        if (fun == "100")command = Xor;
        if (fun == "101") {
            command = x ? sra : srl;
        }
        if (fun == "110")command = Or;
        if (fun == "111")command = And;
        return instruction(rs1,rs2,rd,{0,0},command);
    }
    instruction Jdecode(const std::string &order) {
       int rd = btoi(order.substr(20, 5));
       int imm = btoi(order[0]+  order.substr(12, 8)+ order[11] + order.substr(1, 10)+ '0' );
       opcode command = jal;
       int  height=20;
        if(imm>=1<<20)imm-=1<<21;
        return instruction(0,0,rd,{imm,height},command);
    }
    instruction Idecode(const std::string &order) {
       int rd = btoi(order.substr(20, 5));
       int rs1 = btoi(order.substr(12, 5));
       int imm = btoi(order.substr(0, 12));
       int shamt;
        if(imm>=2048)imm-=4096;
        int height=11;
        opcode command;
        if (order.substr(25, 7) == "1100111") {
            command = jalr;
        } else {
            if (order.substr(25, 7) == "0010011") {
                std::string fun(order.substr(17, 3));
                if (fun == "000")command = addi;
                if (fun == "010")command = slti;
                if (fun == "011")command = sltiu;
                if (fun == "100")command = xori;
                if (fun == "110")command = ori;
                if (fun == "111")command = andi;
                if (fun == "001"){
                    int shamt=btoi(order.substr(7,5));command = slli;
                }
                if (fun == "101") {
                    if (order[1] == '1')command = srai;
                    else {
                        command = srli;
                    }
                    shamt=btoi(order.substr(7,5));
                }
            } else {
                std::string fun(order.substr(17, 3));
                if (fun == "000")command = lb;
                if (fun == "001")command = lh;
                if (fun == "010")command = lw;
                if (fun == "100")command = lbu;
                if (fun == "101")command = lhu;
            }
        }
        instruction i(rs1,0,rd,{imm,height},command);
        i.shamt=shamt;
        return i;

    }
    instruction Bdecode(const std::string &order) {

        int rs1 = btoi(order.substr(12, 5));
        int rs2 = btoi(order.substr(7, 5));
        std::string str=order[24]+order.substr(1, 6)+order.substr(20, 4)  +'0';
        int imm = btoi(order[0]+str);
        if(imm>=4096)imm-=8192;
        int height=12;
        opcode command;
        std::string fun(order.substr(17, 3));
        if (fun == "000")command = beq;
        if (fun == "001")command = bne;
        if (fun == "100")command = blt;
        if (fun == "101")command = bge;
        if (fun == "110")command = bltu;
        if (fun == "111")command = bgeu;
        instruction i(rs1,rs2,0,{imm,height},command);
        return i;
    }
    instruction Sdecode(const std::string &order) {
       int rs1 = btoi(order.substr(12, 5));
       int rs2 = btoi(order.substr(7, 5));
       int imm = btoi( (order.substr(0, 7))+ order.substr(20, 5));
       opcode command;
        if(imm>=2048)imm-=4096;
        int height=11;
        std::string fun(order.substr(17, 3));
        if (fun == "000")command = sb;
        if (fun == "001")command = sh;
        if (fun == "010")command = sw;
        return instruction(rs1,rs2,0,{imm,height},command);
    }
    instruction Udecode(const std::string &order) {
        opcode command;
        if (order[26] == '0')command = auipc;
        else command = lui;
        int rd = btoi(order.substr(20, 5));
        int imm = btoi( order.substr(0, 20));if(imm>=1<<19)imm-=1<<20;//12x0
        int height=31;
        return instruction (0,0,rd,{imm,height},command);
    }
    instruction decode(const std::string &order) {
        std::string opt = order.substr(25, 7);
        codeType type = judgeType(opt);
        instruction ans;
        if (type == R)ans=Rdecode(order);
        if (type == J)ans=Jdecode(order);
        if (type == I)ans=Idecode(order);
        if (type == S)ans=Sdecode(order);
        if (type == U)ans=Udecode(order);
        if (type == B)ans=Bdecode(order);
        ans.type=type;
        return ans;
    }
    class ALU {
    private:
        std::string order;
    public:
        int height=0;
        int memo=0;uint data=0;
        int regi=0;uint tmp=0;int pos=0;
        int v1=0,v2=0;
        unsigned int rs1, rs2;
        unsigned int rd;
        int imm;int shamt;
        codeType type;
        opcode command;

        ALU() {
            for (int i = 0; i < 31; i++)order[i] = '0';
        }

        ALU(const std::string &str) : order(str) {
            memo=regi=0;
        }
        ALU(const instruction& ins){
            memo=regi=0;
            rs1=ins.rs1;
            rs2=ins.rs2;
            rd=ins.rd;
            height=ins.immediate.height;
            imm=ins.immediate.value;
            shamt=ins.shamt;
            command=ins.op;
            pc=ins.PC;

        }


    private:


        //execute
        void ADD() {
            unsigned int cur1 = v1;
            unsigned int cur2 = v2;
            unsigned int cur = cur1 + cur2;
            data = cur;
            regi=1;pc+=4;
        }

        void SUB() {
            //           bool sign=(v1>=v2);
            int cur1 = v1;
            int cur2 = v2;
            int cur(cur1 - cur2);
            data = cur;
            regi=1;pc+=4;


        }

        void XOR() {
             int cur1(v1);
             int cur2(v2);
            int cur(cur1 ^ cur2);
            data = cur;
            regi=1;pc+=4;

        }

        void AND() {
             int cur1(v1);
             int cur2(v2);
            int cur(cur1 & cur2);
            data = cur;
            regi=1;pc+=4;

        }
        void OR(){
            unsigned int cur1(v1);
            unsigned int cur2(v2);
            int cur(cur1 | cur2);
            data = cur;
            regi=1;pc+=4;

        }
        //B
        void BEQ(){
            if(v1==v2){
                pc+=sext(imm, height);
            }else pc+=4;
        }
        void BGE(){
            int cur1=v1;
            int cur2=v2;
            if(cur1>=cur2){
                pc+=sext(imm, height);
            }else{
                pc+=4;
            }

        }
        void BGEU(){

            unsigned int cur1=v1;
            unsigned int cur2=v2;
            if(cur1>=cur2){
                pc+=sext(imm, height);
            }else{
                pc+=4;
            }
        }
        void BLT(){
            int cur1=v1;
            int cur2=v2;
            if(cur1<cur2){
                pc+=sext(imm, height);
            }else{
                pc+=4;
            }
        }
        void BLTU(){
            unsigned int cur1=v1;
            unsigned int cur2=v2;
            if(cur1<cur2){
                pc+=sext(imm, height);
            }else{
                pc+=4;
            }
        }
        void BNE(){
            if(v1!=v2){
                pc+=sext(imm, height);
            }else{
                pc+=4;
            }
        }
        void JAL(){
            data=pc+4;pc+=sext(imm, height);
            regi=1;
        }
        //J
        void JALR(){
            int t=pc+4;
            pc=(v1+sext(imm, height))&~1;
            data=t;
            regi=1;
        }
        //U
        void AUIPC() {
            data = pc + v1;
            regi=1;pc+=4;
        }

        //I
        void ADDI() {
            data = v1 + sext(imm, height);
            regi=1;pc+=4;

        }

        void ANDI() {
            int cur(v1);
            data = cur & sext(imm, height);
            regi=1;pc+=4;

        }
        void ORI() {
            int cur(v1);
            data = cur | sext(imm, height);
            regi=1;pc+=4;

        }
        void XORI() {
            int cur(v1);
            data = cur ^ sext(imm, height);
            regi=1;pc+=4;

        }
        void LB(){
            int x= readMemory(v1+sext(imm, height));
            data=sext(x-((x>>8)<<8),7);
            regi=1;pc+=4;
        }
        void LBU(){
            int x=readMemory(v1+sext(imm, height));
            data=x-((x>>8)<<8);
            regi=1;pc+=4;
        }
        void LH(){
            int x=readMemory(v1+sext(imm, height));
            data=sext(x-((x>>16)<<16),15);
            regi=1;pc+=4;
        }
        void LHU(){
            int x=readMemory(v1+sext(imm, height));
            data=x-((x>>16)<<16);
            regi=1;pc+=4;
        }
        void LW(){
            int x=readMemory(v1+sext(imm, height));
            data=x;
            regi=1;pc+=4;
        }
        void LUI(){
 //todo:
 height=31;
            data = imm << 12;
            regi=1;pc+=4;
        }
        void SB(){
            memo=1;tmp=v2-((v2>>8)<<8);pos=v1+sext(imm, height);pc+=4;
        }
        void SH(){
            memo=1;pos=v1+sext(imm, height);tmp=v2-((v2>>16)<<16);pc+=4;
        }
        void SW(){
            memo=1;
            tmp=v2;
            pos=v1+sext(imm, height);
            pc+=4;
        }
        void SLL(){
            data=v1<<v2;
            regi=1;pc+=4;
        }
        void SLLI(){
            data=v1<<shamt;
            regi=1;pc+=4;
        }
        void SLT(){
            int a1=v1;int a2=v2;
            data=(a1<a2)?1:0;
            regi=1;pc+=4;
        }
        void SLTI(){
            int a1=v1;int a2=imm;
            data=(a1<a2)?1:0;
            regi=1;pc+=4;
        }
        void SLTIU(){
            unsigned int a1=v1;unsigned int a2=imm;
            data=(a1<a2)?1:0;
            regi=1;pc+=4;
        }
        void SLTU(){
            unsigned int a1=v1;unsigned int a2=v2;
            data=(a1<a2)?1:0;
            regi=1;pc+=4;
        }
        void SRA(){
            uint a1=v1,a2=v2;
            data=a1>>a2;
            regi=1;pc+=4;
        }
        void SRAI(){
            data=v1>>(shamt);
            regi=1;pc+=4;
        }
        void SRL(){
            data=v1>>(v2);
            regi=1;pc+=4;
        }
        void SRLI(){
            data=v1>>(unsigned int)shamt;
            regi=1;pc+=4;
        }

    public:
        void execute(uint x1=-1,uint x2=-1) {
            if(x1==-1&&x2==-1){
                v1=reg_prev[rs1];v2=reg_prev[rs2];
            }else{
                v1=x1;v2=x2;
            }
            if (command == add)ADD();
            if (command == sub)SUB();
            if (command == Xor)XOR();
            if (command == And)AND();
            if (command == addi)ADDI();
            if (command == andi)ANDI();
            if (command == xori)XORI();
            if (command == auipc)AUIPC();
            if (command == beq)BEQ();
            if (command == bne)BNE();
            if (command == blt)BLT();
            if (command == bge)BGE();
            if (command == bltu)BLTU();
            if (command == bgeu)BGEU();
            if (command == ori)ORI();
            if (command == jalr)JALR();
            if (command == jal)JAL();
            if (command == lb)LB();
            if (command == lh)LH();
            if (command == lw)LW();
            if (command == lbu)LBU();
            if (command == lhu)LHU();
            if (command == Or)OR();
            if (command == lui)LUI();
            if (command == sb)SB();
            if (command == sh)SH();
            if (command == sw)SW();
            if (command == srl)SRL();
            if (command == sra)SRA();
            if (command==sll)SLL();
            if (command==slli)SLLI();
            if (command == srli)SRLI();
            if (command == srai)SRAI();
            if (command == slti)SLTI();
            if (command == slt)SLT();
            if (command == sltu)SLTU();
            if (command == sltiu)SLTIU();
        }


        void writeMemory(){
           if(memo==1){
               for(int i=0;i<4;i++){
                   mem[pos]=tmp&0xff;
                   tmp>>=8;pos++;
               }
               tmp=pos=0;memo=0;
           }
        }
        void writeRegister(){
            if(regi==1){
                reg_prev[rd]=data;data=0;regi=0;
            }
        }


    };
    uint readMemory(int ps){
        uint x=0;
        for(int i=0;i<4;i++){
            x|=mem[ps+i]<<(i<<3);
        }
        return x;
    }
    int fetchCode(int pos,std::string& ord) {//end return false
        if(pc>last)return -1;
        ord= decToBin(readMemory(pos));
        if(ord=="00001111111100000000010100010011")return 1;
        return 0;
    }
    bool isSL(opcode o){
        return (o==lb||o==lbu||o==lh||o==lhu||o==lw||o==sw||o==sb||o==sh);
    }
    void read() {
        std::string str,strmem;
        std::cin>>str;
        int cnt=0,k=0;
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



};


#endif //RISC_V_SIMULATOR_DECODER_HPP

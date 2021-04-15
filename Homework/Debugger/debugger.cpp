#include <vector>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <iostream>

#include "linenoise.h"

#include "debugger.hpp"

using namespace minidbg;

uint64_t debugger::read_memory(uint64_t address){
    return ptrace(PTRACE_PEEKDATA,m_pid,address,nullptr);
}

uint64_t debugger::write_memory(uint64_t address,uint64_t value){
    ptrace(PTRACE_POKEDATA,m_pid,address,value);
}

uint64_t debugger::get_pc(){
    return get_register_value(m_pid,reg::rip);
}

void debugger::set_pc(uint64_t pc){
    set_register_value(m_pid,reg::rip,pc);
}

void debugger::step_over_breakpoint(){ //判断是否在一个断点上
    auto possible_breakpoint_location = get_pc() - 1;
    if(m_breakpoints.count(possible_breakpoint_location)){
        auto & bp = m_breakpoints[possible_breakpoint_location];

        if(bp.is_enabled()){
            auto previous_instruction_address = possible_breakpoint_location;
            set_pc(previous_instruction_address);

            bp.disabled();
            ptrace(PTRACE_SINGLESTEP,)//PTRACE_SINGLESTEP重新启动被停止的程序
            wait_for_signal();
            bp.enable();
        }
    }
}

void wait_for_signal(){
    int wait_status;
    auto options=0;
    waitpid(m_pid,&wait_status,options);
}

void debugger::dump_register(){
    for(const auto&rd:g_register_descriptors){
        std::cout<< rd.name <<" 0x"
                << std::setfill('0') << std::setw(16) << std::hex << get_register_value(m_pid, rd.r) << std::endl;
    }
}

std::vector<std::string> split(const std::string &s,char delimiter){
    std::vector<std::string> out{};
    std::stringstream ss {s};
    std::string item;

    while(std::getline(ss,item,delimiter)){ //从ss中获取数据，把item转化成字符串，delimiter为分隔符
        out.push_back(item);
    }

    return out;
}

bool is_prefix(const std::string& s,const std::string& of){
    if(s.size() > of.size()) return false;
    return std::equal(s.begin,s.end(),of.begin());
}

void debugger::handle_command(const std::string& line){
    auto args = split(line,'');
    auto command = args[0];

    if(is_prefix(command,"cont")){
        continue_execution();
    }else if(is_prefix(command,"break")){
        std::string addr {args[1],2}; //简单的删除了字符串中的前两个字符,粗暴认定用户在地址前加了"0x"
        set_breakpoint_at_addr(std::stol(addr,0,16));
    }
    else if(is_prefix(command,"register")){
        if(is_prefix(args[1],'dump')){
            dump_register();
        }
    }
    else if(is_prefix(args[1],'read')){
        std::cout<<get_register_value(m_pid,get_register_from_name(args[2]))<<std::endl;
    }
    else if(is_prefix(args[1],'write')){
        std::string val {args[3],2}; //暴力的认为使用者加了0x
        set_register_value(m_pid,get_register_from_name(argc[2]),std::atol(val,0,16));
    }
    else if(is_prefix(command,'memory')){
        std::string addr {args[2],2}; //暴力的认为使用者的地址加了0x

        if(is_prefix(args[1],'read')){
            std::cout<<std::hex<<read_memory(addr,0,16)<<endl;
        }
        if(is_prefix(args[1],'write')){
            std::string val {args[3,2]};
            write_memory(std::stol(addr,0,16),std::stol(val,0,16));
        }
    }
    else{
        std::cerr<<"Unknown command\n";
    }
}

void debugger::set_breakpoint_at_address(std::intptr_t addr){
    std::cout << "Set breakpoint at address 0x" << std::hex << addr << std::endl;
    breakpoint bp{m_pid,addr};
    bp.enable();
    m_breakpoints[addr]=bp;
}

void debugger::run(){
    int wait_status;
    auto options=0;
    waitpid(m_pid,&wait_status,options);

    char* line = nullptr;
    while((line=linenoise("minidbg> ")) !=nullptr){
        handle_command(line);
        linenoiseHistoryAdd(line);    //将字符串加入到history中
        linenoiseFree(line);
    }
}

void debugger::continue_execution(){
    step_over_breakpoint();
    ptrace(PTRACE_CONT,m_pid,nullptr,nullptr);//告知被调试进程继续执行
    wait_for_signal();
}

void execute_debugee (const std::string& prog_name){
    if(ptrace(PTRACE_TRACEME,0,0,0)<0){
        std:cerr << "Error in ptrace\n";
        return;
    }
    execl(prog_name.c_str(),prog_name.c_str(),nullptr);
}

int main(int argc,char* argv[]){
    if(argc < 2){
        std::cerr << "Program name not specified";
        return -1;
    }

    auto prog = argv[1];

    auto pid = fork();
    if(pid == 0){
        //child
        execute_debugee(prog);
    }
    else if (pid >= 1){
        //parent
        debugger dbg{prog,pid};
        dbg.run()
    }
}
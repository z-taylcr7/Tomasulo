#include <iostream>
#include "simulator.hpp"
using namespace Cristiano;
int main(){
	std::ios::sync_with_stdio(false);
    std::cin.tie(0);
    std::cout.tie(0);
 //   freopen("az.out","w",stdout);
//	std::ifstream in("3.txt");
//    std::streambuf *backinbuf = std::cin.rdbuf();
//    std::cin.rdbuf(in.rdbuf()); 
//    std::ofstream out("out.txt");
//	std::streambuf *default_buf=std::cout.rdbuf();
//	std::cout.rdbuf( out.rdbuf() ); 
	simulator console;
	console.scan();
	console.run();
//    std::cout.rdbuf( default_buf );
//	std::cin.rdbuf( backinbuf );    
	return 0;
}

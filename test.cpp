#include "edn.hpp"
#include <string>
#include <iostream>
#include <sstream>

int main() {
  std::string ednString;
  while (true) { 
     std::cout << "edn> ";
     getline(std::cin, ednString);
     if (ednString.length() == 0) {
       std::cout << std::endl;
     } else { 
       edn::EdnNode node = edn::read(ednString);
       std::cout << edn::pprint(node) << std::endl;
    }
  }

  return 0; 
}

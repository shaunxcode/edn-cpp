#include "edn.hpp"

int main() {
  edn::EdnNode node = edn::read("[this is a vector]"); 
  return 0; 
}

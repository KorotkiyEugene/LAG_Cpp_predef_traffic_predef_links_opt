
#include "systemc.h"
#include "LAG_top.h"
#include "parameters.h"

int main(int argc, char* argv[])
{
  //sim_injection_rate = 0.49;
  return sc_main(argc, argv);
}

int sc_main(int argc, char* argv[]) 
{
  
 LAG_top top("TOP");
 sc_start();
 return 0;
}
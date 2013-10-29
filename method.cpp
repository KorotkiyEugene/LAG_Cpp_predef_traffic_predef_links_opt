#include "stdlib.h"
#include "types.h"
#include "destinations.h"
#include "parameters.h"

#include <list>
#include <fstream>
#include <iostream>

using namespace std;

const char command[] = "./run.x";    //name of the model's executable
const char tmp_filename[] = "tmp";   //delete?

const char filename_for_trunk_util[] = "trunk_utils.dat";
const char filename_for_flow_delays[] = "flow_delays.dat";

const char filename_links[] = "links.dat";

const int total_steps = (network_x*network_y*router_trunk_num*2 - network_x*network_y - 4*network_x - 4*network_y)*3;

const int critical_delay = 50;           // This is our optimization goal in clock cycles. 
                                         // Max. values of transport delays for all data flows should be less than this number.

const int max_pl_num_per_trunk = 5;

//links_t links2[network_x][network_y];

list<trunk_util_t> read_utilization_of_trunks(const char* filename);
list<data_flow_t>  read_delays_of_data_flows(const char* filename);
int links_to_file(links_t (&links)[network_x][network_y], const char* filename);
bool is_trunk_belongs_to_flow_with_critical_delay(trunk_util_t trunk, const list<data_flow_t> &flows, int critical_delay);
void init_links(links_t links[network_x][network_y], const destinations_t destinations[network_x][network_y]);
bool are_problem_flows_exist(const list<data_flow_t> &flows, int critical_delay);
void links_to_console(const links_t links[network_x][network_y]);
void delays_of_flows_to_console(const list<data_flow_t> &flows);

int main(int argc, char* argv[]) 
{
  bool problem_flows = true;
  
  list<trunk_util_t> util_of_trunks;
  list<data_flow_t> flows;
  
  init_links(links, destinations);
  
  for (int i = 0; i < total_steps; i++)
  {
    links_to_file(links, filename_links);
  
    links_to_console(links);
    
    cout << "Iteration " << i << endl << endl;
  
    system(command); //perform simulation with distribution of PL in trunks which set with the aid of function above
    
    util_of_trunks = read_utilization_of_trunks(filename_for_trunk_util);
    util_of_trunks.sort();
    
    flows = read_delays_of_data_flows(filename_for_flow_delays);
    flows.sort();
    
    delays_of_flows_to_console(flows);
    
    problem_flows = are_problem_flows_exist(flows, critical_delay);
    
    if (!problem_flows) break; // if there are no problem flows, then exit optimization
  
    list<trunk_util_t> :: iterator p = util_of_trunks.end();
  
    while ( p != util_of_trunks.begin() )   
    {
      p--;
      
      if (is_trunk_belongs_to_flow_with_critical_delay(*p, flows, critical_delay) 
            && links[(*p).x][(*p).y].out[(*p).out_trunk_num] < max_pl_num_per_trunk)
      {
        //here we should increment the quantity of PL in trunk that pointed by p
  
        links[(*p).x][(*p).y].out[(*p).out_trunk_num]++;
    
        switch ((*p).out_trunk_num) //we supposed that input-tile trunks dont increment
                                    //and boderline trunks also dont increment (e.g. NORT and WEST trunks of 0,0 router)
        {
          case SOUTH:  //  | 
                       // \/
          
                      {
                        links[(*p).x][(*p).y + 1].in[NORTH]++;    
                        break;
                      }; 
          
          case WEST:  // <--
                      {
                        links[(*p).x - 1][(*p).y].in[EAST]++;
                        break;
                      };
          
          case NORTH:  //  /\ 
                       //   |
          
                      {
                        links[(*p).x][(*p).y - 1].in[SOUTH]++; 
                        break;
                      };
                      
          
          case EAST:  // -->
                      {
                        links[(*p).x + 1][(*p).y].in[WEST]++;
                        break;
                      };
                      
                                                      
          default: 
                      {
                        //impossible combination -> we should tell about ERROR
                      
                      };
        }
    
        break;
      }
        
    }
    
  } //for
  
  links_to_console(links);
  
 return 0;
}

//****** Reading info about normalized utilization rates of the trunks ******
//***************************************************************************
    
list<trunk_util_t> read_utilization_of_trunks(const char* filename)
{
  list<trunk_util_t> util_of_trunks; 
   
  ifstream in_trunk_util_stream(filename, ios::in | ios::binary);
 
  int size;
 
  if (!in_trunk_util_stream)
  {
    cout << "File opening error!" << endl;
  }
 
  in_trunk_util_stream.read((char*)&size, sizeof(size));
  
  trunk_util_t tmp_trunk_util;
  
  for (int i = 0; i < size; i++)
  {
    in_trunk_util_stream.read((char*)&tmp_trunk_util, sizeof(trunk_util_t));
    util_of_trunks.push_back(tmp_trunk_util);
  }
          
  in_trunk_util_stream.close();
  
  return util_of_trunks;
}

void links_to_console(const links_t links[network_x][network_y])
{
  cout << endl;
  
  for (int x = 0; x < network_x; x++)
  {
    for (int y = 0; y < network_y; y++)
    {
      for (int i = 0; i < router_trunk_num; i++)
        cout << links[x][y].in[i] << " ";
      
      cout << "\t";
      
      for (int i = 0; i < router_trunk_num; i++)
        cout << links[x][y].out[i] << " ";
      
      cout << endl;  
     }   
    
    cout << endl;
  }

}

void delays_of_flows_to_console(const list<data_flow_t> &flows)
{
  list<data_flow_t>:: const_iterator p = flows.end();
  
  while (p != flows.begin() )    
  {
    p--; 
    
    printf("(%1d, %1d) -> (%1d, %1d): max_lat = %1d;\n", 
            (*p).x_src, (*p).y_src, (*p).x_dest, (*p).y_dest, (*p).max_latency
            );
                            
    cout << endl << endl;
  }  

}

void init_links(links_t links[network_x][network_y], const destinations_t destinations[network_x][network_y])
{
  for (int x = 0; x < network_x; x++)
    for (int y = 0; y < network_y; y++)
      for (int i = 0; i < router_trunk_num; i++)
      {
        if (destinations[x][y].dest_num && i == TILE)
          links[x][y].in[i] = destinations[x][y].dest_num;
        else
          links[x][y].in[i] = 1;
          
        links[x][y].out[i] = 1;
      }
}

  
//******* Reading info about delays of data flows in the NoC with LAG *******
//***************************************************************************
  
list<data_flow_t>  read_delays_of_data_flows(const char* filename)
{
  list<data_flow_t> flows;
  
  ifstream delays_of_flows_from_file(filename, ios::in | ios::binary);
  
  if (!delays_of_flows_from_file)
  {
    cout << "File opening error!" << endl;
  }
  
  int size = 0;
  
  delays_of_flows_from_file.read((char*)&size, sizeof(size));
  
  data_flow_t data_flow_tmp;
  
  for (int i = 0; i < size; i++)
  {
    delays_of_flows_from_file.read((char*)&data_flow_tmp, sizeof(data_flow_t));
    flows.push_back(data_flow_tmp);
  }
  
  delays_of_flows_from_file.close();
  
  return flows;
}

int links_to_file(links_t (&links)[network_x][network_y], const char* filename)
{
  if (links)
  {
    ofstream links_to_file(filename_links, ios::out | ios::binary | ios::trunc);
          
    if (!links_to_file)
    {
      cout << "File opening error!" << endl;
      return 1;
    }
  
    links_to_file.write ((char*)links, sizeof(links) );
    links_to_file.close();
    
    return 0;
  } else {
  
    cout << "Error in func. links_to_file() -> 1-st arg. (pointer) links == 0" << endl; 
    
    return 1;
  } 
}

bool is_trunk_belongs_to_flow_with_critical_delay(trunk_util_t trunk, const list<data_flow_t> &flows, int critical_delay)
{
  list<data_flow_t> :: const_iterator p = flows.end(); //here flows should be sorted by rising of their transport delays
                                                       //so at the end of the list situated most "problem" flow
  while (p != flows.begin() )    
  {
    p--;
    
    if ((*p).max_latency >= critical_delay) 
    {
      // here we have a flow with critical delay
      // we should check belonging of trunk to path of this flow
      
      if ( ( trunk.y == (*p).y_src ) && ( trunk.x >= (*p).x_src) &&
           ( trunk.x < (*p).x_dest) )
        return true; 
        
      if ( ( trunk.y == (*p).y_src ) && ( trunk.x <= (*p).x_src) &&
           ( trunk.x > (*p).x_dest) )
        return true;   
      
      if ( (trunk.x == (*p).x_dest ) && (trunk.y <= (*p).y_src) && 
           (trunk.y > (*p).y_dest) )
        return true;
      
      if ( (trunk.x == (*p).x_dest ) && (trunk.y >= (*p).y_src) && 
           (trunk.y < (*p).y_dest) )
        return true;
        
      if ((trunk.x == (*p).x_dest ) && (trunk.y == (*p).y_dest) && trunk.out_trunk_num == TILE)
        return true;  
        
    } else return false;
  
  }

  return false;
  
}

bool are_problem_flows_exist(const list<data_flow_t> &flows, int critical_delay)
{
  //flows should be sorted by growing of max_latency
  
  if (!flows.size() ) return false;
  
  if (flows.back().max_latency >= critical_delay)
    return true;
  else
    return false;

}


  /*list<trunk_util_t> :: iterator p = util_of_trunks.end();
  
  while ( p != util_of_trunks.begin() )   
  {
            p--;
            
            cout << "x= " << (*p).x << endl;
            cout << "y= " << (*p).y << endl;
            cout << "out_trunk_num= " << (*p).out_trunk_num << endl;
            cout << "normalized_trunk_util= " << (*p).normalized_trunk_util << endl;
            cout << endl;
  } 
  
  list<data_flow_t> :: iterator q = flows.end();
  
  cout << endl;
  
  while ( q != flows.begin() )   
  {
    q--;
    
    printf("(%1d, %1d) -> (%1d, %1d): max_lat = %1d;\n", 
            (*q).x_src, (*q).y_src, (*q).x_dest, (*q).y_dest, (*q).max_latency
            );
                            
    cout << endl << endl;
            
  }
  
  */
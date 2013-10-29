#ifndef LAG_MESH_NETWORK_TB_H
#define LAG_MESH_NETWORK_TB_H

#include "LAG_router_tiny.h"
#include "LAG_traffic_sink.h"
#include "LAG_traffic_src.h"

#include <list>
#include <fstream>
#include <iomanip>

#include "types.h"

//using std::stringstream;
using namespace std;

static const int total_packets = sim_measurement_packets;
const char filename_for_trunk_util[] = "trunk_utils.dat";
const char filename_for_flow_delays[] = "flow_delays.dat";  

const char filename_links[] = "links.dat";

class LAG_mesh_network_tb: sc_module
{
  public:
  
  sc_in<bool> clk;
  sc_in<bool> rst_n;
  
  SC_HAS_PROCESS(LAG_mesh_network_tb);
  
  LAG_mesh_network_tb(sc_module_name name_):sc_module(name_), routers(network_y*network_x),
                                            traffic_sinks(network_y*network_x), traffic_sources(network_y*network_x),
                                            total_lat_freq(101)
  {
    startup_message = false;
    reset_complete = false;
    
    if (optimization_mode)
    {
      //reading info about quantity of PLs in trunks from file links.dat
      
      ifstream links_from_file(filename_links, ios::in | ios::binary);
      
      if (!links_from_file)
      {
        cout << "File \"links.dat\" opening error!" << endl;
        sc_stop();
      }
      
      links_from_file.read((char*)links, sizeof(links));
      
      links_from_file.close(); 
    
    }
    
    dummy_inputs_pl_num = links[0][0].in[NORTH]; // <-- N PL in dummy (unconnected) trunks. All dummy trunks should contain N PLs. 
                                                 //     So we can init dummy_inputs_pl_num by the number of PLs in first available dummy trunk
    
    traf_sink_flit = new sc_signal<my_vector<flit_t> >[network_y*network_x];
    traf_src_flit = new sc_signal<my_vector<flit_t> >[network_y*network_x];
    traf_sink_control = new sc_signal<my_vector<bool> >[network_y*network_x];
    traf_src_control = new sc_signal<my_vector<bool> >[network_y*network_x];  
  
    dummy_in_flit = new sc_signal<my_vector<flit_t> >[2*network_y + 2*network_x];
    dummy_out_flit = new sc_signal<my_vector<flit_t> >[2*network_y + 2*network_x];
    dummy_in_control = new sc_signal<my_vector<bool> >[2*network_y + 2*network_x];
    dummy_out_control = new sc_signal<my_vector<bool> >[2*network_y + 2*network_x];
  
    between_router_flit = new sc_signal<my_vector<flit_t> >[2*(network_x - 1)*network_y + 2*(network_y - 1)*network_x];
    between_router_control = new sc_signal<my_vector<bool> >[2*(network_x - 1)*network_y + 2*(network_y - 1)*network_x];
    
    int k = 0;
    int l = 0;
    
    for (int y = 0; y < network_y; y++)
    {
      for (int x = 0; x < network_x; x++)
      {
        stringstream s_router, s_src, s_sink;
        
        //creating router at xy position
        s_router << "ROUTER" << x << y;
        routers[network_x*y + x] = new LAG_router_tiny(s_router.str().c_str(),
                                                  links[x][y], 
                                                  x,
                                                  y,
                                                  router_buf_len,
                                                  router_trunk_num
                                                  );
        
        //creating traffic source at xy position                                          
        s_src << "TRAFFIC_SRC" << x << y;
        
        traffic_sources[network_x*y + x] = new LAG_traffic_src(s_src.str().c_str(),
                                                                    links[x][y].in[TILE],
                                                                    network_x,
                                                                    network_y,
                                                                    x,
                                                                    y,
                                                                    PREDEFINED_TRAFFIC,
                                                                    destinations[x][y],
                                                                    sim_packet_length,
                                                                    source_fifo_length
                                                                    );
        
        //creating traffic sink at xy position                                                            
        s_sink << "TRAFFIC_SINK" << x << y;
        traffic_sinks[network_x*y + x] = new LAG_traffic_sink(s_sink.str().c_str(),
                                                              network_x,
                                                              network_y,
                                                              x,
                                                              y,
                                                              sim_warmup_packets,
                                                              sim_measurement_packets,
                                                              links[x][y].out[TILE]
                                                              );

        
        //connecting traffic source at xy position to signals
        traffic_sources[network_x*y + x] -> clk(clk);
        traffic_sources[network_x*y + x] -> rst_n(rst_n);
        traffic_sources[network_x*y + x] -> cntrl_in(traf_src_control[network_x*y + x]);
        traffic_sources[network_x*y + x] -> flit_out(traf_src_flit[network_x*y + x]);
        
        //connecting traffic sink at xy position to signals
        traffic_sinks[network_x*y + x] -> clk(clk);
        traffic_sinks[network_x*y + x] -> rst_n(rst_n);
        traffic_sinks[network_x*y + x] -> flit_in(traf_sink_flit[network_x*y + x]);
        traffic_sinks[network_x*y + x] -> cntrl_out(traf_sink_control[network_x*y + x]);
        
        
        //connecting router at xy position to traffic source and sink signals
        routers[network_x*y + x] -> clk(clk);
        routers[network_x*y + x] -> rst_n(rst_n);
        routers[network_x*y + x] -> flit_in[TILE](traf_src_flit[network_x*y + x]);
        routers[network_x*y + x] -> flit_out[TILE](traf_sink_flit[network_x*y + x]);
        routers[network_x*y + x] -> cntrl_in[TILE](traf_sink_control[network_x*y + x]);
        routers[network_x*y + x] -> cntrl_out[TILE](traf_src_control[network_x*y + x]);
        
        //systemc forbids leave ports unconnected so we connecting dummy signals 
        //to unused inputs and outputs of router
        // x and y index router in mesh network
        // k index links corresponding to this router - we use two dim. indexing
        //for routers and one dim. (e.g. linear) indexing for links
        if (y == 0)
        {
          routers[network_x*y + x] -> flit_in[NORTH](dummy_in_flit[k]);
          routers[network_x*y + x] -> flit_out[NORTH](dummy_out_flit[k]);
          routers[network_x*y + x] -> cntrl_in[NORTH](dummy_in_control[k]);
          routers[network_x*y + x] -> cntrl_out[NORTH](dummy_out_control[k]);
            
          k++;  
        }
        
        if (x == 0)
        {
          routers[network_x*y + x] -> flit_in[WEST](dummy_in_flit[k]);
          routers[network_x*y + x] -> flit_out[WEST](dummy_out_flit[k]);
          routers[network_x*y + x] -> cntrl_in[WEST](dummy_in_control[k]);
          routers[network_x*y + x] -> cntrl_out[WEST](dummy_out_control[k]); 
            
          k++;
        }
        
        if (x == network_x - 1)
        {
          routers[network_x*y + x] -> flit_in[EAST](dummy_in_flit[k]);
          routers[network_x*y + x] -> flit_out[EAST](dummy_out_flit[k]);
          routers[network_x*y + x] -> cntrl_in[EAST](dummy_in_control[k]);
          routers[network_x*y + x] -> cntrl_out[EAST](dummy_out_control[k]); 
          
          k++;
        }
        
        if (y == network_y - 1)
        {
          routers[network_x*y + x] -> flit_in[SOUTH](dummy_in_flit[k]);
          routers[network_x*y + x] -> flit_out[SOUTH](dummy_out_flit[k]);
          routers[network_x*y + x] -> cntrl_in[SOUTH](dummy_in_control[k]);
          routers[network_x*y + x] -> cntrl_out[SOUTH](dummy_out_control[k]); 
            
          k++;
        }
        
      }
    }
    
    //now connecting routers between each other
    
    //step1 - working with horizontal links
    
    //i and j indexing bidirectional connection between (i,j) and (i+1,j) routers by x dimension
    //l index one directional link between routers (linear indexing)
    for (int j = 0; j < network_y; j++)
    {
      for (int i = 0; i < network_x - 1; i++)
      {
        //connecting flit and control west outputs of (i+1,j) router 
        //to flit and control east inputs of (i,j) router
        routers[network_x*j + i] -> flit_in[EAST](between_router_flit[l]);
        routers[network_x*j + i] -> cntrl_in[EAST](between_router_control[l]);
 
        routers[network_x*j + i + 1] -> flit_out[WEST](between_router_flit[l]);
        routers[network_x*j + i + 1] -> cntrl_out[WEST](between_router_control[l]);
        
        l++;
        
        //connecting flit and control east outputs of (i,j) router 
        //to flit and control west inputs of (i+1,j) router    
        routers[network_x*j + i + 1] -> flit_in[WEST](between_router_flit[l]);
        routers[network_x*j + i + 1] -> cntrl_in[WEST](between_router_control[l]);
 
        routers[network_x*j + i] -> flit_out[EAST](between_router_flit[l]);
        routers[network_x*j + i] -> cntrl_out[EAST](between_router_control[l]); 
        
        l++;
                       
      }
    }  
    
    //step2 - working with vertical links
    
    //i and j indexing bidirectional connection between (i,j) and (i,j+1) routers by y dimension
    //l index one directional link between routers (linear indexing)    
    for (int j = 0; j < network_y - 1; j++)
    {
      for (int i = 0; i < network_x; i++)
      {
        //connecting south outputs of (i,j) router to north inputs of (i, j+1) router
        routers[network_x*(j + 1) + i] -> flit_in[NORTH](between_router_flit[l]);
        routers[network_x*(j + 1) + i] -> cntrl_in[NORTH](between_router_control[l]);
 
        routers[network_x*j + i] -> flit_out[SOUTH](between_router_flit[l]);
        routers[network_x*j + i] -> cntrl_out[SOUTH](between_router_control[l]); 
        
        l++;
        
        //connecting north outputs of (i, j+1) router to south inputs of (i,j) router
        routers[network_x*j + i] -> flit_in[SOUTH](between_router_flit[l]);
        routers[network_x*j + i] -> cntrl_in[SOUTH](between_router_control[l]);
 
        routers[network_x*(j + 1) + i] -> flit_out[NORTH](between_router_flit[l]);
        routers[network_x*(j + 1) + i] -> cntrl_out[NORTH](between_router_control[l]); 
        
        l++;
        
      }
    }
          
    SC_METHOD(main_process);
    sensitive_pos << clk;
  
  }
  
  
  void main_process()
  {
    if (!rst_n)
    {
      reset_complete = false;
      
      sys_time = 0;
      
      if (!startup_message && results_to_console)
      {
        startup_message = true;
        cout << "***********************************************************" << endl;
        cout << "*                   NoC with LAG testing                  *" << endl;
        cout << "***********************************************************" << endl << endl;
        cout << "-- Simulation start" << endl;
        cout << "-- Entering reset state" << endl;
      }
      
      total_latency = total_min_latency = total_max_latency = 0;
      for (int i = 0; i <= 100; i++)
        total_lat_freq[i] = 0;
  
      //initializing dummy inputs by zeros
      my_vector<flit_t> zero_flits(dummy_inputs_pl_num);                     //all fields of flit_t is zero by default
      my_vector<bool>   zero_controls(dummy_inputs_pl_num, false);
      
      for (int i = 0; i < 2*network_y + 2*network_x; i++)
      {
        dummy_in_flit[i].write(zero_flits);
        dummy_in_control[i].write(zero_controls);
      }
      
    }else
    {
      startup_message = false;
      
      sys_time++;
      
      if (!reset_complete && results_to_console)
      {
        reset_complete = true;
        cout << "-- Reset Complete" << endl;
        cout << "-- Entering warmup phase (" << sim_warmup_packets << " packets per node)" << endl;
      }
      
      //cout << "+" << endl;
      
      total_rec_packets = 0; //can be optimized: we could not silly sum all rec packets every clock
                             //but do summation incrementally, every clock cycle adding only newly arriwed packets
      
      for (int y = 0; y < network_y; y++)
        for (int x = 0; x < network_x; x++)
          total_rec_packets += traffic_sinks[network_x*y + x] -> get_stats().rec_packets_count;
    
      //displaying progress of simulation
      if (operation_status)
        if ( ( total_rec_packets % (sim_measurement_packets/20) ) == 0 && total_rec_packets != 0 ) 
        {
          printf("%1d: %1.2f%% complete", sys_time, (float)(total_rec_packets*100)/ (float) (sim_measurement_packets) );
          cout << endl;
        }
        
      if (total_rec_packets >= total_packets)
      {
        //if (results_to_console)
        {
        
        cout << "-- Simulation end" << endl;
        
        total_min_latency = traffic_sinks[0] -> get_stats().min_latency;
        total_max_latency = traffic_sinks[0] -> get_stats().max_latency;
        
        for (int y = 0; y < network_y; y++)
          for (int x = 0; x < network_x; x++)
          {
            total_latency += traffic_sinks[network_x*y + x] -> get_stats().total_latency;
            total_min_latency = std::min(total_min_latency, traffic_sinks[network_x*y + x] -> get_stats().min_latency);
            total_max_latency = std::max(total_max_latency, traffic_sinks[network_x*y + x] -> get_stats().max_latency);
            
            for (int i = 0; i <= 100; i++)
              total_lat_freq[i] += traffic_sinks[network_x*y + x] -> get_stats().lat_freq[i];
          }
        
        cout << fixed << setprecision(2) << endl;
        
        cout << "**********************************************************" << endl;
        cout << "*               Simulation statistics                    *" << endl;
        cout << "**********************************************************" << endl << endl;
        
        cout << "-- Packet Length   = " << sim_packet_length << endl;
        cout << "-- Average Latency = " << (float)(total_latency) / (float)(total_rec_packets) << " (cycles)" << endl;
        cout << "-- Min. Latency    = "<< total_min_latency << ", Max. Latency = " << total_max_latency << endl << endl;
        
        cout << "Average Latencies for packets rec'd at nodes [x,y] and (no. of packets received)" << endl;
        
        float average_latency_x_y = 0;
        
        for (int y = 0; y < network_y; y++)
        {
          for (int x = 0; x < network_x; x++)
          {
            if (traffic_sinks[network_x*y + x] -> get_stats().rec_packets_count != 0)
            {
              average_latency_x_y = (float)(traffic_sinks[network_x*y + x] -> get_stats().total_latency) / (traffic_sinks[network_x*y + x] -> get_stats().rec_packets_count);
              
            } else
            {
              average_latency_x_y = 0;
            }
            
            cout << average_latency_x_y << " (" << traffic_sinks[network_x*y + x] -> get_stats().rec_packets_count << ")\t";
          
          }
          
          cout << endl;
        }

        cout << endl;
        cout << "Flits/cycle sended at each node: (sum of intensities of all flows)" << endl;
        
        vector<int> flits_sent;
        
        int total_flits_sent = 0;
        
        for (int y = 0; y < network_y; y++)
        {
          for (int x = 0; x < network_x; x++)
          {
            flits_sent = traffic_sources[network_x*y + x] -> get_flits_sent();
                
            for (int pl = 0; pl < links[x][y].in[TILE]; pl++)
              total_flits_sent += flits_sent[pl];
            
            if(destinations[x][y].dest_num)
              cout << ((float)total_flits_sent)/sys_time << "\t";
            else
              cout << 0 << "\t";
          
            total_flits_sent = 0;
          } 
          
          cout << endl;
        }  
                
                
        cout << endl;
        cout << "Flits/cycle received at each node: (should approx. injection rate)" << endl;

        int x_y_measure_start = 0;
        int x_y_measure_end = 0;
        int x_y_flit_count = 0;

        for (int y = 0; y < network_y; y++)
        {
          for (int x = 0; x < network_x; x++)
          {
            if (traffic_sinks[network_x*y + x] -> get_stats().measure_start != -1)
            {
              x_y_measure_start = traffic_sinks[network_x*y + x] -> get_stats().measure_start;
              x_y_measure_end = traffic_sinks[network_x*y + x] -> get_stats().measure_end;
              x_y_flit_count = traffic_sinks[network_x*y + x] -> get_stats().rec_flits_count;
              cout << (float)(x_y_flit_count) / (x_y_measure_end - x_y_measure_start)<< "\t";
            } else
              cout << 0 << "\t";
                                                                                             
          }
          
          cout << endl;
        }        
        
        cout << endl;
        
        cout << "Distribution of packet latencies: " << endl;
        cout << "Latency : Frequency (as percentage of total)" << endl;
        cout << "--------------------------" << endl;
        
        int lat_freq_x_y_i = 0;
        
        for (int i = 0; i < 100; i++)
          cout << i << " " << (float)(total_lat_freq[i]) * 100 / total_packets << endl;
        
        cout << "100+ " << (float)(total_lat_freq[100]) * 100 / total_packets << endl << endl;
        
        cout << "Latencies for packet flows (in clock cycles)" << endl;
        cout << "----------------------------------------------------------" << endl;
          
        for (int j = 0; j < network_y; j++)         // destination y
          for (int i = 0; i < network_x; i++)       // destination x
            for (int l = 0; l < network_y; l++)     // source y
              for (int k = 0; k < network_x; k++)   // source x
                if(traffic_sinks[network_x*j + i] -> get_stats().flows[k][l].valid)
                {
                  printf("(%1d, %1d) -> (%1d, %1d): min = %1d;   av = %1d;   max = %1d;\n", 
                            k, l, i, j, traffic_sinks[network_x*j + i] -> get_stats().flows[k][l].min_latency, 
                            traffic_sinks[network_x*j + i] -> get_stats().flows[k][l].total_latency / traffic_sinks[network_x*j + i] -> get_stats().flows[k][l].rec_packets, 
                            traffic_sinks[network_x*j + i] -> get_stats().flows[k][l].max_latency);
                
                  cout << "----------------------------------------------------------";
                  cout << endl;
                }
        
        } // results_to_console
        
        if(gather_info_for_optimization)
        {
          
          //**** Evaluate normalized utilization rate of the trunks of the NoC with LAG ****
          //********************************************************************************
          
          float normalized_trunk_util = 0;
          trunk_util_t trunk_util_tmp;
          
          ofstream out_trunk_util(filename_for_trunk_util, ios::out | ios::binary | ios::trunc);
                              
          if (!out_trunk_util)
          {
            cout << "File opening error!" << endl;
            sc_stop();
          }
          
          int size = network_x*network_y*router_trunk_num;
          
          out_trunk_util.write((char*)&size, sizeof(size) );
        
          for (int x = 0; x < network_x; x++)
            for (int y = 0; y < network_y; y++)
              for (int t = 0; t < router_trunk_num; t++)
              { 
                normalized_trunk_util = 0;
                 
                for (int pl = 0; pl < links[x][y].out[t]; pl++)
                  normalized_trunk_util += routers[network_x*y + x] -> flits_transmitted[t][pl];  //normalized_trunk_util contains quantity of flits that passed through trunk for the time of simulation
               
                normalized_trunk_util /= sys_time;  // normalized_trunk_util contains unnormalized trunk utilization rate - how many flits passed through trunk per clock cycle
              
                normalized_trunk_util /= links[x][y].out[t]; // normalized_trunk_util contains trunk utilization rate normalized by the quantity of data flows passed through the trunk
              
                // here we use sys_time as simulation time. It is unperfect solution because the NoC warmup period included
                // in this time. I should improve this in future making each router able to evaluate the end of warmup phase, e.g.
                // when router passed through particular link 100 flits. In this case each router should start count flits
                // passed through it PLs from the end of warmup period
              
                trunk_util_tmp.x = x;
                trunk_util_tmp.y = y;
                trunk_util_tmp.out_trunk_num = t;
                trunk_util_tmp.normalized_trunk_util = normalized_trunk_util;
                
                out_trunk_util.write((char*)&trunk_util_tmp, sizeof(trunk_util_t) );
              }   
          
          out_trunk_util.close();
          
          //********************************************************************************
          
          
          //************ Evaluate latencies of data flows in the NoC With LAG **************
          //********************************************************************************
          data_flow_t data_flow_tmp;
          
          ofstream delays_of_flows_to_file(filename_for_flow_delays, ios::out | ios::binary | ios::trunc);
          
          if (!delays_of_flows_to_file)
          {
            cout << "File opening error!" << endl;
            sc_stop();
          }
          
          for (int j = 0; j < network_y; j++)         // destination y
          for (int i = 0; i < network_x; i++)       // destination x
            for (int l = 0; l < network_y; l++)     // source y
              for (int k = 0; k < network_x; k++)   // source x
                if(traffic_sinks[network_x*j + i] -> get_stats().flows[k][l].valid)
                {
                  data_flow_tmp.x_src = k;
                  data_flow_tmp.y_src = l;
                  data_flow_tmp.x_dest = i;
                  data_flow_tmp.y_dest = j;
                  data_flow_tmp.max_latency = traffic_sinks[network_x*j + i] -> get_stats().flows[k][l].max_latency;
                  
                  flows.push_back(data_flow_tmp);
                }
          
          size = flows.size();
          
          delays_of_flows_to_file.write((char*)&size, sizeof(size) );
          
          list<data_flow_t> :: iterator q = flows.begin();
          
          while ( q != flows.end() )
          {
            delays_of_flows_to_file.write((char*)&*q, sizeof(data_flow_t) );
            q++;
          }
          
          delays_of_flows_to_file.close();
          
          /*list<data_flow_t> :: iterator q = flows.end();
          
          cout << endl; 
          
          while ( q != flows.begin() )   
          {
            q--;
            
            printf("(%1d, %1d) -> (%1d, %1d): max_lat = %1d;\n", 
                      (*q).x_src, (*q).y_src, (*q).x_dest, (*q).y_dest, (*q).max_latency
                  );
                            
            cout << endl << endl;
          }  */
          
          
          //********************************************************************************
             
        }
                
        sc_stop();
      }
      
    }
  }
  
  
  ~LAG_mesh_network_tb()
  {
    if (traf_sink_flit) delete [] traf_sink_flit; 
    if (traf_src_flit) delete [] traf_src_flit; 
    if (traf_sink_control) delete [] traf_sink_control; 
    if (traf_src_control) delete [] traf_src_control; 
    if (dummy_in_flit) delete [] dummy_in_flit; 
    if (dummy_out_flit) delete [] dummy_out_flit; 
    if (dummy_in_control) delete [] dummy_in_control;
    if (dummy_out_control) delete [] dummy_out_control;
    if (between_router_flit) delete [] between_router_flit;
    if (between_router_control) delete [] between_router_control;  
  }
  
  private:
  
  vector<LAG_router_tiny*>               routers;    
  vector<LAG_traffic_sink*>         traffic_sinks;
  vector<LAG_traffic_src*>     traffic_sources;
  
  sc_signal<my_vector<flit_t> >*    traf_sink_flit;
  sc_signal<my_vector<flit_t> >*    traf_src_flit;
  sc_signal<my_vector<bool> >*      traf_sink_control;
  sc_signal<my_vector<bool> >*      traf_src_control;
  
  sc_signal<my_vector<flit_t> >*    dummy_in_flit;
  sc_signal<my_vector<flit_t> >*    dummy_out_flit;
  sc_signal<my_vector<bool> >*      dummy_in_control;
  sc_signal<my_vector<bool> >*      dummy_out_control;
    
  sc_signal<my_vector<flit_t> >*    between_router_flit;
  sc_signal<my_vector<bool> >*      between_router_control;
  
  bool startup_message;
  bool reset_complete;
  
  int total_rec_packets;
  int total_latency, total_min_latency, total_max_latency;
  vector<int> total_lat_freq;
  
  int sys_time;
  
  int dummy_inputs_pl_num; // pl count in links for dummy (not connected) inputs should be equal this value!!!!!!!!!!!!!!!!!!
  
  list<data_flow_t> flows;
};

#endif
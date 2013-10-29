#ifndef LAG_TRAFFIC_SRC_H
#define LAG_TRAFFIC_SRC_H

#include "systemc.h"
#include "types.h"
#include "LAG_fifo.h"
#include "LAG_flow_control.h"
#include "LAG_route.h"
#include "parameters.h"

#include "types.h"
#include <fstream>

using namespace std;

class LAG_traffic_src: public sc_module
{
  public:

  sc_in<bool>                 clk, rst_n;
  sc_in<my_vector<bool> >     cntrl_in;
  sc_out<my_vector<flit_t> >  flit_out;

  SC_HAS_PROCESS(LAG_traffic_src);

  LAG_traffic_src(sc_module_name name_,
                        int pl_num_ = 2,
                        int x_dim_ = 4,           //mesh network size
                        int y_dim_ = 4,
                        int x_pos_ = 0,           //traffic src position in mesh network
                        int y_pos_ = 0,
                        traffic_t traffic_type_ = RANDOM_TRAFFIC,
                        destinations_t destinations_ = destinations_t(), //destinations for this traffic source
                        int packet_length_ = 4,
                        int source_fifo_length_ = 20,
                        double rate_ = 0.3           //flits injection rate flit/cycle
                        
                        ): sc_module(name_), pl_num(pl_num_), x_dim(x_dim_),
                        y_dim(y_dim_), x_pos(x_pos_), y_pos(y_pos_), 
                        packet_length(packet_length_), rate(rate_),
                        source_fifo_length(source_fifo_length_),
                        destinations(destinations_), packets_to_inject(destinations_.dest_num), 
                        dest_probs(destinations_.dest_num), traffic_type(traffic_type_), 
                        tmp_flit(destinations_.dest_num)
  {
    
    if (traffic_type == PREDEFINED_TRAFFIC && destinations.dest_num > destinations.max_dest_num)
    {
      cout << "ERROR!!! The number of destinations for traffic_src[" << x_pos << ",";
      cout << y_pos << "] is greater than max_dest_num (" << destinations.max_dest_num << ")." << endl;
      sc_stop();
    }
    
    if (traffic_type == PREDEFINED_TRAFFIC && destinations.dest_num)
      if (traffic_type == PREDEFINED_TRAFFIC && destinations.dest_num != pl_num)
      {
        cout << "ERROR!!! destinations.dest_num for traffic_src(" << x_pos << ",";
        cout << y_pos << ") should be equal pl_num." << endl;
      }
    
    
    if (traffic_type == RANDOM_TRAFFIC)
    {
      destinations.dest_num = 1;
      destinations.dests[0].dest_injection_rate = rate;
    }
    
    fifos.resize(destinations.dest_num);
    
    flit_count.resize(destinations.dest_num);
    inject_count.resize(destinations.dest_num);
    flits_buffered.resize(destinations.dest_num);
    flits_sent.resize(destinations.dest_num);
    fifo_ready.resize(destinations.dest_num);
    
    fifo_full = new sc_signal<bool>[destinations.dest_num];
    fifo_empty = new sc_signal<bool>[destinations.dest_num];
    fifo_push = new sc_signal<bool>[destinations.dest_num]; 
    fifo_pop = new sc_signal<bool>[destinations.dest_num];
    
    fifo_in = new sc_signal<flit_t>[destinations.dest_num]; 
    fifo_out = new sc_signal<flit_t>[destinations.dest_num];
    
    
    
    for (int i = 0; i < destinations.dest_num; i++) 
    {
      dest_probs[i] = 10000 * destinations.dests[i].dest_injection_rate / packet_length;
      
      stringstream fifo_name;
      fifo_name << "TRAFFIC_GEN_FIFO" << i;
      
      fifos[i] = new LAG_fifo(fifo_name.str().c_str(), 2 + source_fifo_length);
      fifos[i] -> clk(clk);
      fifos[i] -> rst_n(rst_n);
      fifos[i] -> data_in(fifo_in[i]);
      fifos[i] -> data_out(fifo_out[i]);
      fifos[i] -> full(fifo_full[i]);
      fifos[i] -> empty(fifo_empty[i]);
      fifos[i] -> push(fifo_push[i]);
      fifos[i] -> pop(fifo_pop[i]); 
    }
    
    flow_control = new LAG_flow_control("TRAFFIC_GEN_FLOW_CONTROL", pl_num, router_buf_len);
    flow_control -> clk(clk);
    flow_control -> rst_n(rst_n);
    flow_control -> channel_credits(cntrl_in);
    flow_control -> flits_valid(flits_valid);
    flow_control -> pl_status(out_blocked);
    
    SC_METHOD(main_method);
    sensitive_pos << clk;
    
    SC_METHOD(out_driver);
    sensitive << out_blocked;
    for (int i = 0; i < destinations.dest_num; i++)
      sensitive << fifo_out[i] << fifo_empty[i];
    
    //tmp = router_buf_len;
    
  }
  
  
  void main_method()
  { 
    //in
    bool fifo_full_i;
    bool fifo_empty_i;
    //my_vector<bool> out_blocked_i;
    
    if(!rst_n)
    {
      //init vars
      prob = 10000*rate/packet_length;
      sys_time = 0;
       
      seed = x_pos*50 + y_pos;
      srand(seed);
      
      for (int i = 0; i < destinations.dest_num; i++)
      {
        packets_to_inject[i] = 0;
        
        flit_count[i] = 0;
        inject_count[i] = 0;
        flits_buffered[i] = 0;
        flits_sent[i] = 0;
        fifo_ready[i] = 1;
      }
      
    }else             
    {
      /*if (cntrl_in.read()[0] && x_pos == 0 && y_pos == 1)
      {
        cout << sc_time_stamp() << "+++++++++++++++++++++++++++ Credit come ; Blocked = " << out_blocked.read()[0] << " Count = " << ++tmp << endl;
      }
      
      if (flit_out.read()[0].valid && x_pos == 0 && y_pos == 1)
      {
        cout << sc_time_stamp() << "++++ Flit out ---- Blocked = " << out_blocked.read()[0] << " Count = " << --tmp << endl;
        
      }  */
      
      for (int i = 0; i < destinations.dest_num; i++)
      {
        //reading and checking input signals
        fifo_full_i = fifo_full[i].read();
        
        if(fifo_pop[i].read())        
          flits_sent[i]++; 
          
        if (fifo_push[i].read()) flits_buffered[i]++;  
      
        //
        // start buffering next flit when there is room in FIFO
        //
        if ( ( flits_buffered[i] - flits_sent[i] ) <= source_fifo_length ) // !!!!!!!!!!!!!!!!!!!!! вместо packet_length поставить fifo_length
  	     fifo_ready[i] = 1;
        else
         fifo_ready[i] = 0;
         
        if (fifo_ready[i])

  	    // **********************************************************
  	    // Random Injection Process
  	    // **********************************************************
            
        if (rand()%10000 < dest_probs[i])
  		    packets_to_inject[i]++;
     
  	    if ( !(fifo_ready[i] && packets_to_inject[i]) )
  	    {
          fifo_push[i].write(false);
          
        }else
  	    {
  	    
          flit_count[i]++;
  	    
          fifo_push[i].write(true);
  
          //
          // Send Head Flit
          //
          if (flit_count[i] != 1)
          {
            tmp_flit[i].head = false;
          }else
          {
  	     
  	       tmp_flit[i] = zero_flit;
  	       
  	       inject_count[i]++;
  	       
           if (traffic_type == RANDOM_TRAFFIC)
           {
             //
    	       // set random displacement to random destination
    	       //
             
              while ( (x_pos == x_dest) && (y_pos == y_dest) )
              {
                  // don't send to self...
  	             x_dest = rand() % x_dim;
  	             y_dest = rand() % y_dim;
  	          }
           }  else
           {  
              //predefined sources traffic
           
              x_dest = destinations.dests[i].dest_addr_x;
              y_dest = destinations.dests[i].dest_addr_y;
           }
  	       
  	       tmp_flit[i].route.dx = x_dest - x_pos;
  	       tmp_flit[i].route.dy = y_dest - y_pos;
  	       
  	       tmp_flit[i].head = true;
  	      
  	       tmp_flit[i].debug.x_dest = x_dest;
  	       tmp_flit[i].debug.y_dest = y_dest;
  	       tmp_flit[i].debug.x_src = x_pos;
  	       tmp_flit[i].debug.y_src = y_pos;
  	       
           tmp_flit[i].debug.inject_time = sys_time;
           
          }
        
          tmp_flit[i].debug.flit_id = flit_count[i];
          tmp_flit[i].debug.packet_id = inject_count[i];
          
          if (flit_count[i] == packet_length)
          {
            tmp_flit[i].tail = true;
            packets_to_inject[i]--;
            flit_count[i] = 0;
            //fifo_ready = 0;
          }
          
          fifo_in[i].write(tmp_flit[i]);
          
        } //if (fifo_ready && injecting_packet)
  	 
  	    //fifo_in[i].write(tmp_flit);
	    
      }
      
      sys_time++;
       
    } //if(!rst_n)
  }
  
  
  void out_driver()
  {
    //in
    flit_t  fifo_out_i;
    my_vector<bool> out_blocked_i;
    
    //out
    my_vector<flit_t>   flit_out_o(pl_num);
    my_vector<bool>     flits_valid_o(pl_num);
    
    if(!rst_n)
    {
      my_vector<flit_t> zero_flits(pl_num);
      my_vector<bool>   false_bools(pl_num, false); 
      
      flit_out = zero_flits;
      flits_valid = false_bools;
    
    }else
    {
      out_blocked_i = out_blocked.read();
      sc_assert(out_blocked_i.size() == pl_num);
        
      for (int i = 0; i < destinations.dest_num; i++)
      {
        //reading and checking input signals
        fifo_out_i = fifo_out[i].read();  
        
        //processing input signals and generating output signals
        flit_out_o[i] = fifo_out_i.head ? route(fifo_out_i) : fifo_out_i;
        flit_out_o[i].valid = !fifo_empty[i].read() && !out_blocked_i[i];
        flits_valid_o[i] = flit_out_o[i].valid;
          
        //writing output signals
        if (flit_out_o[i].valid)
          fifo_pop[i].write(true);
        else
          fifo_pop[i].write(false);
          
      }
      
        flit_out.write(flit_out_o);
        flits_valid.write(flits_valid_o);
        
    }     
  
  }
  
  vector<int> get_flits_sent()
  {
    return flits_sent;
  }
  
  ~LAG_traffic_src()
  {    
    for (int i = 0; i < destinations.dest_num; i++) 
      if(fifos[i]) delete fifos[i];
    
    if (flow_control) delete flow_control;
    
    if (fifo_in) delete [] fifo_in;
    if (fifo_out) delete [] fifo_out;
    if (fifo_full) delete [] fifo_full;
    if (fifo_empty) delete [] fifo_empty;
    if (fifo_push) delete [] fifo_push;
    if (fifo_pop) delete [] fifo_pop;
  }

  private:
  
  //int tmp;

  int pl_num;
  int x_dim;
  int y_dim;
  int x_pos;
  int y_pos;
  int packet_length;
  int source_fifo_length;
  double rate;   
  
  double prob;                   //probability of packet creation
  long long int sys_time;
  
  vector<int> flit_count;
  vector<int> inject_count;
  vector<int> flits_buffered;
  vector<int> flits_sent;
  vector<bool> fifo_ready;
  
  int seed; //rng seed value 
  int x_dest, y_dest;
  
  vector<flit_t> tmp_flit;
  const flit_t zero_flit; //all member vars are zeros after creation 
  
  vector<LAG_fifo*>             fifos;
  LAG_flow_control*     flow_control;
  
  //fifo signals
  sc_signal<flit_t>* fifo_in;
  sc_signal<flit_t>* fifo_out;
  
  sc_signal<bool>* fifo_full; 
  sc_signal<bool>* fifo_empty; 
  sc_signal<bool>* fifo_push; 
  sc_signal<bool>* fifo_pop;
  
  //flow control signals
  sc_signal<my_vector<bool> > flits_valid;
  sc_signal<my_vector<bool> > out_blocked;
  
  destinations_t destinations;
                                   
  vector<int> packets_to_inject;
  vector<float> dest_probs;
  
  traffic_t traffic_type;

};


#endif
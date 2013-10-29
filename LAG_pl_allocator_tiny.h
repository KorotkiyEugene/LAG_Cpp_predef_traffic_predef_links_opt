#ifndef LAG_PL_ALLOCATOR_TINY_H
#define LAG_PL_ALLOCATOR_TINY_H

#include "systemc.h"
#include "LAG_round_robin_arb.h"
#include "LAG_matrix_arb.h"
#include "types.h"
#include "LAG_tree_arb.h"

class LAG_pl_allocator_tiny
{
public:
  
  void reset()
  {
    for(int i = 0; i < n_trunks * max_links_in; i++)
      arbs1[i] -> reset();
      
    for(int i = 0; i < n_trunks * max_links_out; i++)
      arbs2[i] -> reset();
  }
    
  void allocate(const vector<vector<bool> > & req_i, 
                  const vector<vector<int> > & out_trunk_i,
                  const vector<vector<bool> > & pl_status_i, 
                  vector<vector<int> > & pl_new_o,
                  vector<vector<bool> > & pl_new_valid_o,
                  vector<vector<bool> > & pl_allocated_o
                  )
  {    
      assert(req_i.size() == n_trunks);
      assert(out_trunk_i.size() == n_trunks);
      assert(pl_status_i.size() == n_trunks);
      
      assert(pl_new_o.size() == n_trunks);
      assert(pl_new_valid_o.size() == n_trunks);
      assert(pl_allocated_o.size() == n_trunks);
      
      //cout << "Allocator in" << endl;
      
      //1st level arbitration
      for(int i = 0; i < n_trunks; i++)
      {
         
        assert(req_i[i].size() == links.in[i]);
        assert(out_trunk_i[i].size() == links.in[i]);
        assert(pl_status_i[i].size() == links.out[i]);
        
        assert(pl_new_o[i].size() == links.in[i]);
        assert(pl_new_valid_o[i].size() == links.in[i]);
        assert(pl_allocated_o[i].size() == links.out[i]);
        
        for(int j = 0; j < links.in[i]; j++)   
        {
          //cout << "Req[" << i << "][" << j << "] = " << req_i[i*n_pls + j] << endl;
         
          for(int k = 0; k < max_links_out; k++)
          {
            out_trunk_tmp = out_trunk_i[i][j];
         
            if (k < links.out[out_trunk_tmp])
              arbs1[i*max_links_in + j] -> requests[k] = req_i[i][j] && pl_status_i[out_trunk_tmp][k]; //arrays of arbs can be realized based on vectors
            else
              arbs1[i*max_links_in + j] -> requests[k] = false;
          }
          
          arbs1[i*max_links_in + j] -> update_grants();
        }    
      } 
     
      //for(int i = 0; i < n_trunks; i++)
        //for(int j = 0; j < n_pls; j++)  
          //for(int l = 0; l < n_pls; l++)
            //cout << "arbs1_grant[" << i << "][" << j << "][" << l << "] = " << arbs1[i*n_pls + j] -> grants[l] << endl;
      
      //2nd level arbitration
  
      for(int k = 0; k < n_trunks; k++) //output trunks indexing
      {
        for(int l = 0; l < links.out[k]; l++) //output physical links indexing
        {
          
          pl_allocated_o[k][l] = false;
          
          for(int i = 0; i < n_trunks; i++) //input trunks indexing
          {
            for(int j = 0; j < max_links_in; j++)  //input physical links indexing
            {
              if (j < links.in[i])
                arbs2[k*max_links_out + l] -> requests[i*max_links_in + j] = arbs1[i*max_links_in + j] -> grants[l] && (out_trunk_i[i][j] == k) ; 
              else
                arbs2[k*max_links_out + l] -> requests[i*max_links_in + j] = false;
                
              //cout << "arbs2_req[" << k << "][" << l << "][" << i << "][" << j << " ] = " << arbs2[k*n_pls + l] -> requests[i*n_pls + j] << endl;
              
              if(!pl_allocated_o[k][l]) //some optimization - output pl is allocated if at least one of input pls wants to connect with it. So we can ignore all susequent requests for this output pl. We interested only in first request
                //pl_allocated_o[k*n_pls + l] = pl_allocated_o[k*n_pls + l] || arbs2[k*n_pls + l] -> requests[i*n_pls + j];
                pl_allocated_o[k][l] = arbs2[k*max_links_out + l] -> requests[i*max_links_in + j];
            }
          }
                     
        }
      }
      
      pl_new_valid_o = false_bools;   
      pl_new_o = zero_ints;
      
      for(int k = 0; k < n_trunks; k++) //output trunks indexing
      {
        for(int l = 0; l < links.out[k]; l++) //output physical links indexing
        {
          arbs2[k*max_links_out + l] -> update_grants();
          
          for(int i = 0; i < n_trunks; i++) //input trunks indexing
          {
            for(int j = 0; j < links.in[i]; j++)  //input physical links indexing
            {
              pl_new_valid_o[i][j] = pl_new_valid_o[i][j] || arbs2[k*max_links_out + l] -> grants[i*max_links_in + j];
              
              if (arbs2[k*max_links_out + l] -> grants[i*max_links_in + j])
                pl_new_o[i][j] = l;  //(It is small 'L' after '=', not '1')
            }
          }
        
        }
      }
      
      /*for(int i = 0; i < n_trunks; i++)
        for(int j = 0; j < n_pls; j++)
          cout << "pl_new_valid[" << i << "][" << j << "] = " << pl_new_valid_o[i*n_pls + j] << endl;
      
      for(int i = 0; i < n_trunks; i++)
        for(int j = 0; j < n_pls; j++)    
          cout << "pl_new[" << i << "][" << j << "] = " << pl_new_o[i*n_pls + j] << endl;
         
      for(int i = 0; i < n_trunks; i++)
        for(int j = 0; j < n_pls; j++)    
          cout << "pl_allocated[" << i << "][" << j << "] = " << pl_allocated_o[i*n_pls + j] << endl;   */
      
         
      //cout << "pl_new" << pl_new_o << endl;
          //cout << "pl_allocated" << pl_allocated_o << endl;
      //cout << "Allocator out" << endl; */
   
    update_priorities();
        
  }
    
  LAG_pl_allocator_tiny(string name_,
                    links_t links_,  
                    int n_trunks_ = 5                    
                    ): name(name_), n_trunks(n_trunks_),
                    false_bools(n_trunks_), zero_ints(n_trunks_), links(links_) 
  {
    max_links_in = max_pls_in();
    max_links_out = max_pls_out();
        
    arbs1 = new LAG_round_robin_arb*[n_trunks * max_links_in]; //here we create more arbiters then actually using later - this should be optimizes
    //arbs1 = new LAG_matrix_arb*[n_trunks * n_pls];
    for(int i = 0; i < n_trunks * max_links_in; i++)
      arbs1[i] = new LAG_round_robin_arb(max_links_out);
      //arbs1[i] = new LAG_matrix_arb(n_pls);
    
    //arbs2 = new LAG_tree_arb*[n_trunks * n_pls];
    arbs2 = new LAG_round_robin_arb*[n_trunks * max_links_out];
    
    for(int i = 0; i < n_trunks * max_links_out; i++)
      //arbs2[i] = new LAG_tree_arb(n_trunks * n_pls, n_pls);
      arbs2[i] = new LAG_round_robin_arb(n_trunks * max_links_in);  
    
    for(int i = 0; i < n_trunks; i++)  
    {
      false_bools[i].resize(links.in[i], false);
      zero_ints[i].resize(links.in[i], 0);
    }
    
    reset();  
  }                                         
  
  ~LAG_pl_allocator_tiny()
  { 
    
    for(int i = 0; i < n_trunks * max_links_in; i++)
      if(arbs1[i]) delete arbs1[i];
    if(arbs1) delete [] arbs1;
    
    for(int i = 0; i < n_trunks * max_links_out; i++)
      if(arbs2[i]) delete arbs2[i];
    if(arbs2) delete [] arbs2;
  
  }

private:

  void update_priorities()  //should be called on posedge of each clock cycle
  {
    for (int i = 0; i < n_trunks; i++)
      for (int j = 0; j < links.in[i]; j++)
      {
        arbs1[i*max_links_in + j] -> update_priorities();
      }
      
    for (int i = 0; i < n_trunks; i++)
      for (int j = 0; j < links.out[i]; j++)
      {
        arbs2[i*max_links_out + j] -> update_priorities();
      }       
  } 
  
  int max_pls_in()
  {
    int result = 0;
    
    for (int i = 0; i < n_trunks; i++)
      result = std::max(links.in[i], result);       
    
    return result;        
  }     
  
  int max_pls_out()
  {
    int result = 0;
    
    for (int i = 0; i < n_trunks; i++)
      result = std::max(links.out[i], result);  
    
    return result;        
  }      
 
private:  
  
  string name;                      //module name
  int n_trunks;                     //number of trunks in router
  int max_links_in;
  int max_links_out;
  
  int out_trunk_tmp;
  
  LAG_round_robin_arb** arbs1;
  //LAG_matrix_arb** arbs1;
  LAG_round_robin_arb** arbs2;
  //LAG_tree_arb** arbs2;
  
  my_vector<vector<bool> > false_bools;
  my_vector<vector<int> > zero_ints;
  
  links_t links;
  
};

#endif
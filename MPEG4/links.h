  
  #include "parameters.h"

  struct links_t {
  
    int in[router_trunk_num];
    int out[router_trunk_num];
  
  };

	links_t links[network_x][network_y] = {
                                            
                                            { // x = 0
                                                
                                                { // y = 0
                                                  
                                                  {1, 1, 1, 1, 3}, {1, 1, 1, 1, 1} 
                                                    
                                                },
                                                { // y = 1
                                                
                                                  {1, 1, 1, 1, 7}, {1, 1, 1, 1, 1}
                                                  
                                                },
                                                { // y = 2
                                                
                                                  {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}
                                                  
                                                }
                                            },
//-----------------------------------------------------------------------------------------------------------------
                                            { // x = 1
                                                { // y = 0
                                                
                                                  {1, 1, 1, 1, 3}, {1, 1, 1, 1, 1} 
                                                    
                                                },
                                                { // y = 1
                                                
                                                  {1, 1, 1, 1, 5}, {1, 1, 1, 1, 1}
                                                  
                                                },
                                                { // y = 2
                                                
                                                  {1, 1, 1, 1, 2}, {1, 1, 1, 1, 1}
                                                  
                                                }
                                            },
//-----------------------------------------------------------------------------------------------------------------
                                            { // x = 2
                                                { // y = 0
                                                  
                                                  {1, 1, 1, 1, 2}, {1, 1, 1, 1, 1}
                                                  
                                                },
                                                { // y = 1
                                                  
                                                  {1, 1, 1, 1, 6}, {1, 1, 1, 1, 1}
                                                  
                                                },
                                                { // y = 2
                                                  
                                                  {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}
                                                  
                                                }
                                            },
//-----------------------------------------------------------------------------------------------------------------
                                            { // x = 3
                                                { // y = 0
                                                  
                                                  {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}
                                                  
                                                },
                                                { // y = 1
                                                  
                                                  {1, 1, 1, 1, 2}, {1, 1, 1, 1, 1}
                                                  
                                                },
                                                { // y = 2
                                                  
                                                  {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}
                                                  
                                                }
                                            }
                                            };   
//-----------------------------------------------------------------------------------------------------------------

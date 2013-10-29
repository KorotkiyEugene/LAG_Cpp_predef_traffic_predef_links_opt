    #ifndef DESTINATIONS_H
    #define DESTINATIONS_H
    
    #include "parameters.h"
    
    enum traffic_t {RANDOM_TRAFFIC, PREDEFINED_TRAFFIC};
    
    struct dest_t {
                      int dest_addr_x;           // [x,y] - address in mesh network
                      int dest_addr_y;
                      float dest_injection_rate;  // in flit/(clock cycle)
    };
    
    
    struct destinations_t {
    
                      static const int max_dest_num = 4;
                   
                      
                      int dest_num;                // number of destinations for this traffic source
                      dest_t dests[max_dest_num];
                      
                      /* destinations_t()
                      {
                        dest_num = 0;
                      } */ 
    };


	
    // 1-st dim. - network x
    // 2-st dim. - network y
     

    const destinations_t destinations[network_x][network_y] = {
                                            { // x = 0
                                                
                                                { // y = 0
                                                  
                                                  2, { {1, 1, 0.44}, {1, 1, 0.44}}  
                                                    
                                                },
                                                { // y = 1
                                                
                                                  3, { {0, 0, 0.445}, {0, 0, 0.445}, {3, 1, 0.12} }
                                                  
                                                },
                                                { // y = 2
                                                
                                                  2, { {0, 1, 0.45}, {0, 1, 0.45} }    
                                                  
                                                }
                                            },
//-----------------------------------------------------------------------------------------------------------------
                                            { // x = 1
                                                { // y = 0
                                                
                                                  2, { {0, 0, 0.04}, {3, 0, 0.04} }   
                                                    
                                                },
                                                { // y = 1
                                                
                                                  1, { {2, 1, 0.75} }
                                                  
                                                },
                                                { // y = 2
                                                
                                                  2, { {0, 2, 0.45}, {0, 2, 0.45} }  
                                                  
                                                }
                                            },
//-----------------------------------------------------------------------------------------------------------------
                                            { // x = 2
                                                { // y = 0
                                                  
                                                  3, { {3, 0, 0.24}, {2, 1, 0.63}, {2, 1, 0.63} }  
                                                  
                                                },
                                                { // y = 1
                                                  
                                                  1, { {3, 0, 0.78} }
                                                  
                                                },
                                                { // y = 2
                                                  
                                                  2, { {1, 2, 0.45}, {1, 2, 0.45} }
                                                  
                                                }
                                            },
//-----------------------------------------------------------------------------------------------------------------
                                            { // x = 3
                                                { // y = 0
                                                  
                                                  1, { {2, 0, 0.78} }  
                                                  
                                                },
                                                { // y = 1
                                                  
                                                  1, { {0, 2, 0.07} }
                                                  
                                                },
                                                { // y = 2
                                                  
                                                  1, { {2, 2, 0.18} }  
                                                  
                                                }
                                            }
                                            };   
//-----------------------------------------------------------------------------------------------------------------
#endif

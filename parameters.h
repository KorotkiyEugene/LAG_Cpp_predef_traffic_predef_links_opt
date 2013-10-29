#ifndef PARAMETERS_H
#define PARAMETERS_H

const bool debug = false;                // if true - writes info about each link traversing for each flit; very detailed info; usefull for NoC debugging
const bool flit_sequence_check = true;   // Enables checking the sequence of flits transmitting through each channel. Can be disabled for verified NoC.
                                         // Disabling could slightly increase simulation speed

const bool optimization_mode = false;     // if true - model operates as part of optimization system to reach the goal described in method.cpp. 
                                         // if false - just one simulation run. Model takes info about trunks from links.h and writes results to console
                                         
const bool operation_status = !optimization_mode;      //if true - outputs percents of received packets for each traffic sink

const bool gather_info_for_optimization = optimization_mode;
const bool results_to_console = !optimization_mode;            //if false - no data to console

const int router_buf_len = 4;
const int router_trunk_num = 5;

const int network_y = 3;
const int network_x = 4;

const int sim_packet_length = 5;
const int sim_warmup_packets = 200;
const int sim_measurement_packets = 25000;

const int source_fifo_length = sim_packet_length*5;

#endif // PARAMETERS_H

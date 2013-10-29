#ifndef PARAMETERS_H
#define PARAMETERS_H

const bool debug = false;                 //if true - outputs info about each link traversing for each flit; very detailed info
const bool flit_sequence_check = true;

const bool optimization_mode = true;    //if true - model operates as part of optimization system. Model takes info about quantity of PL in trunks from file and writes info about trunk normalized utilization and delays of data streams to files
                                         //if false - model takes info about trunks from links.h and outputs results to console

const bool operation_status = !optimization_mode;      //if true - outputs percents of received packets for each traffic sink

const bool gather_info_for_optimization = optimization_mode;
const bool results_to_console = !optimization_mode;            //if false - no data to console

const int router_buf_len = 4;
const int router_trunk_num = 5;

const int network_y = 3;
const int network_x = 4;

const double sim_injection_rate = 0.6;
const int sim_packet_length = 5;
const int sim_warmup_packets = 200;
const int sim_measurement_packets = 25000;

const int source_fifo_length = sim_packet_length*5;

#endif // PARAMETERS_H

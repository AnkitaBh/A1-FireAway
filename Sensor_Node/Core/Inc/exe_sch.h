/*
 * exe_sch.h
 *
 *  Created on: Nov 16, 2022
 *      Author: karenabruzzo
 */

#ifndef INC_EXE_SCH_H_
#define INC_EXE_SCH_H_

void check_data_schedule(mixnet_node_config_t *config, mixnet_address parent_addr, data_sch_config_t sch_config, data_config_t *sensor_db);
void exe_sch();

#endif /* INC_EXE_SCH_H_ */

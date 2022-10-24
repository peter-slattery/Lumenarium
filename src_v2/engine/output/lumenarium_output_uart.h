/* date = March 30th 2022 9:57 am */

#ifndef LUMENARIUM_OUTPUT_UART_H
#define LUMENARIUM_OUTPUT_UART_H

internal u8* output_com_uart_init();
internal void output_com_uart_build(App_State* state, u32 assembly_id, Assembly_Strip* strip, u8* method_data, Output_Data_Queue* queue);

#endif //LUMENARIUM_OUTPUT_UART_H

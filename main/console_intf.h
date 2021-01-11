#ifndef DASH_CONSOLE_INTF_H
#define DASH_CONSOLE_INTF_H

/*********************
 *      INCLUDES
 *********************/

#include <esp_console.h>
#include <argtable3/argtable3.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void console_intf_init( void );
void console_intf_start( void );
void console_intf_stop( void );
void console_register_commands
    (
    esp_console_cmd_t     * cmds,
    int                     cnt
    );
#endif //DASH_CONSOLE_INTF_H

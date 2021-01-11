
/*********************
 *      INCLUDES
 *********************/
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <pubsub.h>

#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"

#include "linenoise/linenoise.h"

#include "driver/uart.h"

#include "console_intf.h"

/*********************
 *      DEFINES
 *********************/
#define TAG             "CONSOLE_INTF"
#define PROMPT_STR      CONFIG_IDF_TARGET

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *     GLOBALS
 **********************/

/**********************
 *     CONSTANTS
 **********************/

/**********************
 *    PROTOTYPES
 **********************/
_Noreturn static void console_task( void * params );
static void initialize_console(void);

void console_intf_init( void )
{
    // Initialize Console
    initialize_console();

    // Register Commands
    esp_console_register_help_command();
}

void console_intf_start( void )
{
    // Start Console Task
    xTaskCreate( console_task, "console_task", 4 * 1024, NULL, 12, NULL );
}

void console_intf_stop( void )
{
    esp_console_deinit();
}

_Noreturn static void console_task( void * params )
{
    const char* prompt = LOG_COLOR_I PROMPT_STR "> " LOG_RESET_COLOR;

    while(1) {
        char* line = linenoise(prompt);

        if( line ) {
            /* Add the command to the history if not empty*/
            if (strlen(line) > 0) {
                linenoiseHistoryAdd(line);
            }

            /* Try to run the command */
            int ret;
            esp_err_t err = esp_console_run(line, &ret);
            if (err == ESP_ERR_NOT_FOUND) {
                printf("Unrecognized command\n");
            } else if (err == ESP_ERR_INVALID_ARG) {
                // command was empty
            } else if (err == ESP_OK && ret != ESP_OK) {
                printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
            } else if (err != ESP_OK) {
                printf("Internal error: %s\n", esp_err_to_name(err));
            }
            /* linenoise allocates line buffer on the heap, so need to free it */
            linenoiseFree(line);
        }
    }

    ESP_LOGE(TAG, "Error or end-of-input, terminating console");
}

static void initialize_console(void)
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .source_clk = UART_SCLK_REF_TICK,
    };
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM,
                                         256, 0, 0, NULL, 0) );
    ESP_ERROR_CHECK( uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_args = 8,
            .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
            .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

//    /* Configure linenoise line completion library */
//    /* Enable multiline editing. If not set, long commands will scroll within
//     * single line.
//     */
//    linenoiseSetMultiLine(1);
//
//    /* Tell linenoise where to get command completions and hints */
//    linenoiseSetCompletionCallback(&esp_console_get_completion);
//    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);
//
//    /* Set command history size */
//    linenoiseHistorySetMaxLen(100);
//
//    /* Don't return empty lines */
//    linenoiseAllowEmpty(false);
}

void console_register_commands
    (
    esp_console_cmd_t     * cmds,
    int                     cnt
    )
{
    for( int i = 0; i < cnt; i++ ) {
        ESP_ERROR_CHECK( esp_console_cmd_register( &cmds[i] ));
    }
}

/*********************
 *      INCLUDES
 *********************/
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include <pubsub.h>

#include "esp_log.h"

#include "speedometer_gauge.h"
#include "stepper_gauge.h"
#include "console_intf.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "SPEEDO"

#define SPEED_MPH_MIN       0
#define SPEED_MPH_MAX       80

#define SPEED_DEG_MIN       0
#define SPEED_DEG_MAX       87

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *     GLOBALS
 **********************/

static bool             g_stepper_init_finished;

static struct {
    struct arg_int *speed;
    struct arg_end *end;
    } g_speed_args;

static struct {
    struct arg_lit *reset;
    struct arg_end *end;
    } g_reset_args;

static struct {
    struct arg_int *degree;
    struct arg_end *end;
    } g_degree_args;

/**********************
 *     COMMANDS
 **********************/
static int speed_cmd(int argc, char **argv);
static int reset_cmd(int argc, char **argv);
static int degree_cmd(int argc, char **argv);

static esp_console_cmd_t  g_commands[] =
    {
    /*            command              help                                     hint        function                args */
    {   "speed",        "Set Speedometer Value",            NULL,       speed_cmd,              &g_speed_args },
    {   "reset",        "Reset Speedometer To 0 Degrees",   NULL,       reset_cmd,              &g_reset_args },
    {   "degree",       "Set Speedometer To Degree",        NULL,       degree_cmd,             &g_degree_args }
    };

#define COMMANDS_CNT        ( sizeof(g_commands)/sizeof(g_commands[0]) )

/**********************
 *     CONSTANTS
 **********************/

/**********************
 *     MACROS
 **********************/
#define mph_to_deg( _spd )      (float)( ( ( _spd ) * SPEED_DEG_MAX ) / SPEED_MPH_MAX )

/**********************
 *    PROTOTYPES
 **********************/
_Noreturn static void msg_task( void * params );

void speedometer_gauge_init( void )
{
    ESP_LOGI(TAG, "Init");

    g_stepper_init_finished = false;

    // Setup Arguments
    g_speed_args.speed = arg_intn(NULL, NULL, "<int>", SPEED_MPH_MIN, SPEED_MPH_MAX, "mph");
    g_speed_args.end = arg_end(2);

    g_reset_args.reset = arg_lit0(NULL, NULL, NULL);
    g_reset_args.end = arg_end(2);

    g_degree_args.degree = arg_intn(NULL, NULL, "<int>", SPEED_DEG_MIN, SPEED_DEG_MAX, "degrees");
    g_degree_args.end = arg_end(2);

    // Register Commands
    console_register_commands( g_commands, COMMANDS_CNT );
}

void speedometer_gauge_start( void )
{
    ESP_LOGI(TAG, "Start");

    xTaskCreatePinnedToCore(msg_task, "speedometer_gauge_msg_task", 4096*2, NULL, 0, NULL, 1);
}

void speedometer_gauge_stop( void )
{
}

_Noreturn static void msg_task( void * params )
{
    ps_subscriber_t *s = ps_new_subscriber(10, STRLIST( "stepper" ));

    ps_msg_t *msg = NULL;

    while(true) {
        msg = ps_get( s, -1 );
        if( msg != NULL) {
            if( 0 == strcmp( "stepper.started", msg->topic ) ) {
                ESP_LOGI(TAG, "Speedo Stepper Started");
            }
            else if( 0 == strcmp( "stepper.finished", msg->topic ) ) {
                ESP_LOGI(TAG, "Speedo Stepper Finished");

                if( false == g_stepper_init_finished ) {
                    g_stepper_init_finished = true;

                    // Return to Gauge Min
                    stepper_gauge_set_degree( mph_to_deg(SPEED_MPH_MIN) );
                }
            }
            else if( 0 == strcmp( "stepper.ready", msg->topic ) ) {
                ESP_LOGI(TAG, "Speedo Stepper Ready");
                if( false == g_stepper_init_finished ) {
                    // Set to Gauge Max
                    stepper_gauge_set_degree( mph_to_deg(SPEED_MPH_MAX) );
                }
            }
        }
    }
}


static int speed_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &g_speed_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, g_speed_args.end, argv[0]);
        return 1;
    }

    ESP_LOGI(TAG, "Set Speedometer To: %d mph",
             g_speed_args.speed->ival[0]);

    stepper_gauge_set_degree( mph_to_deg( g_speed_args.speed->ival[0] ) );
    return 0;
}


static int reset_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &g_reset_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, g_reset_args.end, argv[0]);
        return 1;
    }

    ESP_LOGI(TAG, "Reset Speedometer");

    stepper_gauge_reset();
    return 0;
}

static int degree_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &g_degree_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, g_degree_args.end, argv[0]);
        return 1;
    }

    ESP_LOGI(TAG, "Set Speedometer To %d degrees",
             g_degree_args.degree->ival[0]);

    stepper_gauge_set_degree( g_degree_args.degree->ival[0] );
    return 0;
}

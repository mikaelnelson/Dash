#ifndef DASH_STEPPER_GAUGE_H
#define DASH_STEPPER_GAUGE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/
#define STEPPER_DEGREE_MIN  0
#define STEPPER_DEGREE_MAX  220

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void stepper_gauge_start( void );
void stepper_gauge_stop( void );
void stepper_gauge_set_degree( float degree );
void stepper_gauge_reset( void );

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif //DASH_STEPPER_GAUGE_H
